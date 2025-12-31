// Configuration
const API_BASE = '/api';
const CANVAS_SIZE = 50;
const POLL_INTERVAL = 1000; // 1 second
const PIXEL_SIZE = 10; // Base pixel size in pixels

// State
let sessionId = localStorage.getItem('sessionId') || generateSessionId();
let userId = null;
let selectedColor = 0;
let selectedMood = 2;
let zoomLevel = 1;
let panX = 0;
let panY = 0;
let cooldownActive = false;
let cooldownEnd = 0;
let lastCanvasUpdate = 0;
let lastEpisodeUpdate = 0;
let lastSeasonUpdate = 0;
let pollingInterval = null;

// Canvas
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');

// Color palette (matches backend)
const colors = [
    '#000000', '#FF0000', '#0000FF', '#00FF00',
    '#FFFF00', '#FF00FF', '#00FFFF', '#FF8000',
    '#8000FF', '#008000', '#808080', '#FFC0CB',
    '#A52A2A', '#FFD700', '#40E0D0', '#FFFFFF'
];

// Initialize
function init() {
    setupEventListeners();
    setupColorPalette();
    loadSession();
    drawCanvas();
}

// Generate session ID
function generateSessionId() {
    const id = 'sess_' + Math.random().toString(36).substr(2, 16) + Date.now().toString(36);
    localStorage.setItem('sessionId', id);
    return id;
}

// Setup event listeners
function setupEventListeners() {
    // Canvas click
    canvas.addEventListener('click', handleCanvasClick);
    
    // Zoom controls
    document.getElementById('zoom-in').addEventListener('click', () => {
        const oldZoom = zoomLevel;
        const oldVisibleWidth = CANVAS_SIZE / oldZoom;
        const oldVisibleHeight = CANVAS_SIZE / oldZoom;
        const centerX = panX + oldVisibleWidth / 2;
        const centerY = panY + oldVisibleHeight / 2;
        
        zoomLevel = Math.min(zoomLevel * 1.5, 5);
        
        const newVisibleWidth = CANVAS_SIZE / zoomLevel;
        const newVisibleHeight = CANVAS_SIZE / zoomLevel;
        panX = centerX - newVisibleWidth / 2;
        panY = centerY - newVisibleHeight / 2;
        
        fetchCanvas();
    });
    
    document.getElementById('zoom-out').addEventListener('click', () => {
        const oldZoom = zoomLevel;
        const oldVisibleWidth = CANVAS_SIZE / oldZoom;
        const oldVisibleHeight = CANVAS_SIZE / oldZoom;
        const centerX = panX + oldVisibleWidth / 2;
        const centerY = panY + oldVisibleHeight / 2;
        
        zoomLevel = Math.max(zoomLevel / 1.5, 0.5);
        
        const newVisibleWidth = CANVAS_SIZE / zoomLevel;
        const newVisibleHeight = CANVAS_SIZE / zoomLevel;
        panX = centerX - newVisibleWidth / 2;
        panY = centerY - newVisibleHeight / 2;
        
        fetchCanvas();
    });
    
    document.getElementById('reset-view').addEventListener('click', () => {
        zoomLevel = 1;
        panX = 0;
        panY = 0;
        fetchCanvas();
    });
    
    // Mood selector
    document.getElementById('mood-selector').addEventListener('change', (e) => {
        selectedMood = parseInt(e.target.value);
    });
    
    // Auth
    document.getElementById('login-btn').addEventListener('click', handleLogin);
    document.getElementById('register-btn').addEventListener('click', handleRegister);
    document.getElementById('logout-btn').addEventListener('click', handleLogout);
    document.getElementById('guest-btn').addEventListener('click', handleGuest);
    document.getElementById('header-login-btn').addEventListener('click', () => {
        showLoggedOutState();
    });
    document.getElementById('show-register-btn').addEventListener('click', () => {
        document.getElementById('login-form').style.display = 'none';
        document.getElementById('register-form').style.display = 'block';
    });
    document.getElementById('show-login-btn').addEventListener('click', () => {
        document.getElementById('register-form').style.display = 'none';
        document.getElementById('login-form').style.display = 'block';
    });
    
    // Chat
    document.getElementById('send-chat').addEventListener('click', sendChat);
    document.getElementById('chat-message').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') sendChat();
    });
    
    // Export
    document.getElementById('export-png').addEventListener('click', exportPNG);
    document.getElementById('export-video').addEventListener('click', exportVideo);
    document.getElementById('view-history').addEventListener('click', viewHistory);
}

// Setup color palette
function setupColorPalette() {
    const palette = document.getElementById('color-palette');
    colors.forEach((color, index) => {
        const swatch = document.createElement('div');
        swatch.className = 'color-swatch';
        swatch.style.backgroundColor = color;
        swatch.dataset.color = index;
        
        if (index === selectedColor) {
            swatch.classList.add('selected');
        }
        
        swatch.addEventListener('click', () => {
            document.querySelectorAll('.color-swatch').forEach(s => s.classList.remove('selected'));
            swatch.classList.add('selected');
            selectedColor = index;
        });
        
        palette.appendChild(swatch);
    });
}

// Canvas click handler
function handleCanvasClick(e) {
    if (cooldownActive) {
        alert('Please wait for cooldown!');
        return;
    }
    
    const rect = canvas.getBoundingClientRect();
    const clickX = e.clientX - rect.left;
    const clickY = e.clientY - rect.top;
    
    // Convert to canvas coordinates
    const pixelX = Math.floor((clickX / (PIXEL_SIZE * zoomLevel)) + panX);
    const pixelY = Math.floor((clickY / (PIXEL_SIZE * zoomLevel)) + panY);
    
    if (pixelX >= 0 && pixelX < CANVAS_SIZE && pixelY >= 0 && pixelY < CANVAS_SIZE) {
        placePixel(pixelX, pixelY);
    }
}

// Place pixel
async function placePixel(x, y) {
    try {
        const response = await fetch(`${API_BASE}/place_pixel`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                x, y,
                color: selectedColor,
                mood: selectedMood,
                sessionId: sessionId
            })
        });
        
        const data = await response.json();
        
        if (data.success) {
            // Start cooldown
            const cooldownTime = userId ? 5 : 10; // 5s for logged in, 10s for guest
            cooldownEnd = Date.now() + (cooldownTime * 1000);
            cooldownActive = true;
            updateCooldownDisplay();
            
            // Refresh canvas
            fetchCanvas();
        } else {
            alert(data.error || 'Failed to place pixel');
        }
    } catch (error) {
        console.error('Error placing pixel:', error);
    }
}

// Update cooldown display
function updateCooldownDisplay() {
    const cooldownDiv = document.getElementById('cooldown');
    
    if (cooldownActive) {
        const remaining = Math.ceil((cooldownEnd - Date.now()) / 1000);
        
        if (remaining <= 0) {
            cooldownActive = false;
            cooldownDiv.textContent = 'Ready!';
            cooldownDiv.classList.remove('active');
        } else {
            cooldownDiv.textContent = `Wait ${remaining}s`;
            cooldownDiv.classList.add('active');
            setTimeout(updateCooldownDisplay, 100);
        }
    } else {
        cooldownDiv.textContent = 'Ready!';
        cooldownDiv.classList.remove('active');
    }
}

// Auth functions
async function handleLogin() {
    const email = document.getElementById('login-email').value;
    const password = document.getElementById('login-password').value;
    
    try {
        const response = await fetch(`${API_BASE}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            sessionId = data.sessionId;
            userId = data.userId;
            localStorage.setItem('sessionId', sessionId);
            showLoggedInState();
        } else {
            alert(data.error || 'Login failed');
        }
    } catch (error) {
        console.error('Login error:', error);
    }
}

async function handleRegister() {
    const email = document.getElementById('register-email').value;
    const username = document.getElementById('register-username').value;
    const password = document.getElementById('register-password').value;
    
    try {
        const response = await fetch(`${API_BASE}/register`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email, username, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            sessionId = data.sessionId;
            userId = data.userId;
            localStorage.setItem('sessionId', sessionId);
            showLoggedInState();
        } else {
            alert(data.error || 'Registration failed');
        }
    } catch (error) {
        console.error('Registration error:', error);
    }
}

function handleLogout() {
    sessionId = generateSessionId();
    userId = null;
    localStorage.removeItem('sessionId');
    showLoggedOutState();
}

function handleGuest() {
    userId = null;
    showCanvasScreen(true);
}

function showCanvasScreen(isGuest = false) {
    document.getElementById('login-screen').style.display = 'none';
    document.getElementById('canvas-screen').style.display = 'block';
    document.getElementById('logout-btn').style.display = isGuest ? 'none' : 'block';
    document.getElementById('header-login-btn').style.display = isGuest ? 'block' : 'none';
    document.getElementById('guest-note').style.display = isGuest ? 'block' : 'none';
    document.getElementById('chat-message').disabled = isGuest;
    document.getElementById('send-chat').disabled = isGuest;
    startPolling();
}

function showLoggedInState() {
    showCanvasScreen(false);
}

function showLoggedOutState() {
    document.getElementById('canvas-screen').style.display = 'none';
    document.getElementById('login-screen').style.display = 'flex';
    stopPolling();
}

function loadSession() {
    if (userId) {
        showLoggedInState();
    } else {
        showLoggedOutState();
    }
}

// Chat
async function sendChat() {
    const input = document.getElementById('chat-message');
    const message = input.value.trim();
    
    if (!message) return;
    
    if (!userId) {
        alert('Please login to chat');
        return;
    }
    
    console.log('Sending chat:', message, 'sessionId:', sessionId);
    try {
        const response = await fetch(`${API_BASE}/chat`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ message, sessionId })
        });
        
        const data = await response.json();
        console.log('Chat send response:', data);
        if (data.success) {
            input.value = '';
            fetchChat();
        } else {
            alert('Failed to send: ' + (data.error || 'Error'));
        }
    } catch (error) {
        console.error('Chat error:', error);
    }
}

// Polling
function startPolling() {
    if (pollingInterval) return; // Already polling
    pollingInterval = setInterval(async () => {
        await fetchCanvas();
        await fetchEpisode();
        await fetchSeason();
        await fetchQuests();
        await fetchChat();
    }, POLL_INTERVAL);
    
    // Initial fetch
    fetchCanvas();
    fetchEpisode();
    fetchSeason();
    fetchQuests();
    fetchChat();
}

function stopPolling() {
    if (pollingInterval) {
        clearInterval(pollingInterval);
        pollingInterval = null;
    }
}

// Fetch canvas
async function fetchCanvas() {
    try {
        const visibleWidth = Math.ceil(CANVAS_SIZE / zoomLevel);
        const visibleHeight = Math.ceil(CANVAS_SIZE / zoomLevel);
        panX = Math.max(0, Math.min(panX, CANVAS_SIZE - visibleWidth));
        panY = Math.max(0, Math.min(panY, CANVAS_SIZE - visibleHeight));
        
        const response = await fetch(`${API_BASE}/canvas?x=${panX}&y=${panY}&width=${visibleWidth}&height=${visibleHeight}&t=${Date.now()}`);
        const data = await response.json();
        
        console.log('Fetched canvas with', data.pixels ? data.pixels.length : 0, 'pixels');
        if (data.pixels) {
            updateCanvas(data.pixels);
        }
    } catch (error) {
        console.error('Error fetching canvas:', error);
    }
}

// Update canvas
function updateCanvas(pixels) {
    console.log('Drawing', pixels.length, 'pixels on canvas');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    pixels.forEach(pixel => {
        const x = pixel.x;
        const y = pixel.y;
        const color = colors[pixel.color] || '#FFFFFF';
        
        ctx.fillStyle = color;
        ctx.fillRect(
            (x - panX) * PIXEL_SIZE * zoomLevel,
            (y - panY) * PIXEL_SIZE * zoomLevel,
            PIXEL_SIZE * zoomLevel,
            PIXEL_SIZE * zoomLevel
        );
    });
}

// Draw canvas
function drawCanvas() {
    canvas.width = CANVAS_SIZE * PIXEL_SIZE * zoomLevel;
    canvas.height = CANVAS_SIZE * PIXEL_SIZE * zoomLevel;
    
    ctx.imageSmoothingEnabled = false;
    fetchCanvas();
}

// Fetch episode info
async function fetchEpisode() {
    try {
        const response = await fetch(`${API_BASE}/episode?t=${Date.now()}`);
        const data = await response.json();
        
        console.log('Fetched episode:', data.episodeNumber, 'time:', data.timeRemaining);
        // if (data.timestamp > lastEpisodeUpdate) {
            // lastEpisodeUpdate = data.timestamp;
            document.getElementById('episode-number').textContent = `Episode ${data.episodeNumber}`;
            
            const minutes = Math.floor(data.timeRemaining / 60);
            const seconds = data.timeRemaining % 60;
            document.getElementById('time-remaining').textContent = 
                `${minutes}:${seconds.toString().padStart(2, '0')}`;
        // }
    } catch (error) {
        console.error('Error fetching episode:', error);
    }
}

// Fetch season
async function fetchSeason() {
    try {
        const response = await fetch(`${API_BASE}/season?t=${Date.now()}`);
        const data = await response.json();
        
        // if (data.timestamp > lastSeasonUpdate) {
            // lastSeasonUpdate = data.timestamp;
            document.getElementById('season').textContent = `Season: ${data.season}`;
        // }
    } catch (error) {
        console.error('Error fetching season:', error);
    }
}

// Fetch quests
async function fetchQuests() {
    try {
        const response = await fetch(`${API_BASE}/quests?t=${Date.now()}`);
        const data = await response.json();
        
        const questsDiv = document.getElementById('quests');
        questsDiv.innerHTML = '';
        
        data.quests.forEach(quest => {
            const div = document.createElement('div');
            div.className = `quest ${quest.completed ? 'completed' : ''}`;
            
            div.innerHTML = `
                ${quest.description}
                ${quest.progress}/${quest.target} ${quest.completed ? '✓' : ''}
            `;
            
            questsDiv.appendChild(div);
        });
    } catch (error) {
        console.error('Error fetching quests:', error);
    }
}

// Fetch chat
async function fetchChat() {
    try {
        const response = await fetch(`${API_BASE}/chat?t=${Date.now()}`);
        const data = await response.json();
        
        console.log('Fetched chat:', data.messages ? data.messages.length : 0, 'messages');
        const chatDiv = document.getElementById('chat-messages');
        chatDiv.innerHTML = '';
        
        data.messages.forEach(msg => {
            const div = document.createElement('div');
            div.className = 'chat-message';
            div.innerHTML = `${msg.username}: ${msg.message}`;
            chatDiv.appendChild(div);
        });
        
        chatDiv.scrollTop = chatDiv.scrollHeight;
    } catch (error) {
        console.error('Error fetching chat:', error);
    }
}

// Export functions
async function exportPNG() {
    try {
        const response = await fetch(`${API_BASE}/export_png`);
        const blob = await response.blob();
        
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'canvas.png';
        a.click();
        
        window.URL.revokeObjectURL(url);
    } catch (error) {
        console.error('Error exporting PNG:', error);
        alert('Failed to export PNG');
    }
}

async function exportVideo() {
    if (!confirm('Generate video? This may take a minute...')) return;
    
    try {
        const response = await fetch(`${API_BASE}/export_video`);
        
        if (!response.ok) {
            const data = await response.json();
            alert(data.error || 'Failed to generate video');
            return;
        }
        
        const blob = await response.blob();
        
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'canvas_replay.mp4';
        a.click();
        
        window.URL.revokeObjectURL(url);
    } catch (error) {
        console.error('Error exporting video:', error);
        alert('Failed to export video. Ensure FFmpeg is installed on the server.');
    }
}

async function viewHistory() {
    try {
        const response = await fetch(`${API_BASE}/history`);
        const data = await response.json();
        
        let html = 'Previous Episodes:\n\n';
        data.episodes.forEach(ep => {
            const date = new Date(ep.timestamp * 1000).toLocaleDateString();
            html += `Episode ${ep.episodeNumber} - ${date}\n`;
        });
        
        alert(html || 'No history available');
    } catch (error) {
        console.error('Error fetching history:', error);
    }
}

// Start the app
init();