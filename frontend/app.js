const recordBtn = document.getElementById('recordBtn');
const sendTextBtn = document.getElementById('sendTextBtn');
const textInput = document.getElementById('textInput');
const statusText = document.getElementById('statusText');
const responseBox = document.getElementById('responseBox');
const guruReply = document.getElementById('guruReply');
const guruOrb = document.querySelector('.guru-orb');

const API_BASE = 'https://happiness-guriu.onrender.com/api'; // Live Render Backend IP
let mediaRecorder;
let audioChunks = [];
let isRecording = false;

// Initialize MediaRecorder for voice capturing
async function initAudio() {
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        mediaRecorder = new MediaRecorder(stream);
        
        mediaRecorder.ondataavailable = event => {
            audioChunks.push(event.data);
        };

        mediaRecorder.onstop = async () => {
            const audioBlob = new Blob(audioChunks, { type: 'audio/webm' });
            audioChunks = [];
            await sendVoiceToBackend(audioBlob);
        };
    } catch (err) {
        console.error("Error accessing microphone:", err);
        statusText.innerText = "Microphone access denied.";
    }
}

// Handle Record Button
recordBtn.addEventListener('click', () => {
    if (!mediaRecorder) {
        initAudio().then(toggleRecording);
    } else {
        toggleRecording();
    }
});

function toggleRecording() {
    if (!isRecording) {
        mediaRecorder.start();
        isRecording = true;
        recordBtn.classList.add('recording');
        guruOrb.classList.add('listening');
        statusText.innerText = "Listening to your soul...";
        recordBtn.innerHTML = '<i class="fas fa-stop"></i>';
    } else {
        mediaRecorder.stop();
        isRecording = false;
        recordBtn.classList.remove('recording');
        guruOrb.classList.remove('listening');
        statusText.innerText = "Guru is processing...";
        recordBtn.innerHTML = '<i class="fas fa-microphone"></i>';
    }
}

// Handle Text sending
sendTextBtn.addEventListener('click', async () => {
    const message = textInput.value.trim();
    if (!message) return;
    
    textInput.value = '';
    statusText.innerText = "Guru is processing...";
    guruOrb.classList.add('listening');
    
    try {
        const response = await fetch(`${API_BASE}/chat`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ message })
        });
        
        const data = await response.json();
        handleGuruResponse(data);
    } catch (error) {
        statusText.innerText = "Connection lost to the Guru.";
        console.error(error);
    } finally {
        guruOrb.classList.remove('listening');
    }
});

async function sendVoiceToBackend(audioBlob) {
    statusText.innerText = "Whispering to the winds...";
    const formData = new FormData();
    formData.append('audio', audioBlob, 'voice.webm');
    
    try {
        const response = await fetch(`${API_BASE}/voice`, {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        handleGuruResponse(data);
    } catch (error) {
        statusText.innerText = "Connection lost to the Guru.";
        console.error(error);
    }
}

function handleGuruResponse(data) {
    statusText.innerText = "Guru has spoken.";
    responseBox.style.display = 'block';
    guruReply.innerText = data.reply;
    
    // In actual hardware setup, the NodeMCU plays the audio.
    // If you want the phone to also play it for testing:
    /*
    if(data.audio_url) {
        const audio = new Audio(`http://localhost:8000${data.audio_url}`);
        audio.play();
    }
    */
}
