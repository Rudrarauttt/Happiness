from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.responses import FileResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
import os
import io
import uuid
from groq import Groq

# Kokoro TTS (Assuming standard model usage, fallback structure included)
try:
    from gtts import gTTS
except ImportError:
    print("Please install gtts: pip install gTTS")

app = FastAPI(title="Happiness Guru API", description="Backend for Sadguru Physical AI Model")

# Enable CORS for the frontend app
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Set Groq API key from environment
# os.environ["GROQ_API_KEY"] = "your_groq_api_key"
groq_client = Groq()

AUDIO_OUTPUT_DIR = "audio_outputs"
os.makedirs(AUDIO_OUTPUT_DIR, exist_ok=True)

latest_audio_id = "none"

class ChatRequest(BaseModel):
    message: str

# Guru System Prompt
SADHGURU_PROMPT = """
You are the Happiness Guru, an AI embodying the wisdom, tone, and intellect of Sadhguru. 
Your responses should be profound, logical, slightly poetic, and aimed at providing spiritual and practical clarity.
Keep your answers brief (under 50 words) as it will be spoken out loud by a physical model.
"""

def generate_sadhguru_response(user_text: str) -> str:
    try:
        completion = groq_client.chat.completions.create(
            model="llama-3.3-70b-versatile", # Great for versatile logic and chat
            messages=[
                {"role": "system", "content": SADHGURU_PROMPT},
                {"role": "user", "content": user_text}
            ],
            temperature=0.7,
            max_tokens=150,
        )
        return completion.choices[0].message.content
    except Exception as e:
        print(f"Groq API Error: {e}")
        return "The universe is experiencing a temporary block. Try again later."

def generate_audio(text: str, filename: str):
    filepath = os.path.join(AUDIO_OUTPUT_DIR, filename)
    try:
        tts = gTTS(text=text, lang='hi', tld='co.in') # Using an Indian dialect for Sadhguru tone
        tts.save(filepath)
        
        # We also save it as "latest.mp3" so the ESP32 can always poll the same URL
        latest_path = os.path.join(AUDIO_OUTPUT_DIR, "latest.mp3")
        import shutil
        shutil.copy(filepath, latest_path)
    except Exception as e:
        print(f"TTS Error: {e}")
    return filepath

@app.post("/api/chat")
async def chat(request: ChatRequest):
    """
    Takes a text message, gets Sadguru's wisdom from Groq, and generates Audio context.
    """
    guru_text = generate_sadhguru_response(request.message)
    
    # Generate unique audio filename
    audio_filename = f"response_{uuid.uuid4().hex}.mp3"
    global latest_audio_id
    latest_audio_id = audio_filename
    generate_audio(guru_text, audio_filename)
    
    return {
        "reply": guru_text,
        "audio_url": f"/api/audio/{audio_filename}"
    }

@app.post("/api/voice")
async def handle_voice(audio: UploadFile = File(...)):
    """
    Receives voice recording from the phone UI, transcribes via Groq Whisper, 
    and then generates Sadguru response and audio.
    """
    file_bytes = await audio.read()
    
    # Needs a filename ending in a valid extension for Groq Whisper
    temp_filename = f"temp_input_{uuid.uuid4().hex}.wav"
    with open(temp_filename, "wb") as f:
        f.write(file_bytes)
        
    try:
        # 1. Groq STT (Speech to Text)
        with open(temp_filename, "rb") as bf:
            transcription = groq_client.audio.transcriptions.create(
              file=(temp_filename, bf.read()),
              model="whisper-large-v3-turbo",
            )
        user_text = transcription.text
        
        # 2. Get Guru Response
        guru_text = generate_sadhguru_response(user_text)
        
        # 3. Generate Audio
        audio_filename = f"response_{uuid.uuid4().hex}.mp3"
        global latest_audio_id
        latest_audio_id = audio_filename
        generate_audio(guru_text, audio_filename)
        
        # Cleanup temp file
        os.remove(temp_filename)
        
        return {
            "transcription": user_text,
            "reply": guru_text,
            "audio_url": f"/api/audio/{audio_filename}"
        }
    except Exception as e:
        if os.path.exists(temp_filename):
            os.remove(temp_filename)
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/api/audio/{filename}")
async def get_audio(filename: str):
    """
    Endpoint for the ESP32 or the Frontend to download/stream the generated audio.
    """
    filepath = os.path.join(AUDIO_OUTPUT_DIR, filename)
    if os.path.exists(filepath):
        return FileResponse(filepath, media_type="audio/mpeg")
    return {"error": "File not found"}

@app.get("/api/status")
def status():
    return {"status": "Guru is awake.", "latest_id": latest_audio_id}

# Serve the frontend files at the root
# Ensure this is placed after all API routes
import pathlib
frontend_dir = pathlib.Path(__file__).parent.parent / "frontend"
app.mount("/", StaticFiles(directory=str(frontend_dir), html=True), name="frontend")

if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 8000))
    uvicorn.run(app, host="0.0.0.0", port=port)
