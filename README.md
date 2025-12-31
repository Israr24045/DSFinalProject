# Season Canvas

A collaborative pixel-art web application where users draw together on a shared canvas that resets every 15 minutes.

## Features

- 50×50 pixel collaborative canvas
- 15-minute episodes with automatic reset
- Seasonal visual effects (Bloom, Frost, Warm, Calm)
- Pixel moods (Happy, Sad, Calm, Energetic)
- Community quests
- Real-time chat for registered users
- Guest mode (read-only chat, longer cooldown)
- Canvas snapshots every 10 seconds
- Export episode as PNG or video replay
- History of past episodes

## Requirements

### System Requirements
- Linux Mint (or any Linux), macOS, or Windows
- C++17 compiler (g++ or clang++)
- CMake 3.10+
- FFmpeg (for video export)

### Install Dependencies

#### Linux (Ubuntu/Mint)
```bash
sudo apt-get update
sudo apt-get install build-essential cmake ffmpeg
```

#### macOS
```bash
brew install cmake ffmpeg
```

#### Windows
- Install MinGW or Visual Studio with C++ support
- Download FFmpeg from https://ffmpeg.org/download.html
- Add FFmpeg to PATH

## Build Instructions

1. **Download required header libraries**

Create a `libs` directory in `backend/`:
```bash
cd backend
mkdir -p libs
cd libs
```

Download cpp-httplib:
```bash
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

Download stb_image_write:
```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```

Move them to backend/:
```bash
mv httplib.h ../
mv stb_image_write.h ../
cd ../..
```

2. **Build the project**

```bash
mkdir build
cd build
cmake ..
make
```

3. **Run the server**

```bash
./season_canvas
```

The server will start on `http://localhost:8080`

## Usage

1. Open your browser and navigate to `http://localhost:8080`
2. You can browse as a guest or register/login
3. Select a color and mood, then click pixels to place them
4. Watch the canvas evolve with seasonal effects
5. Complete community quests for visual rewards
6. At episode end, download PNG or video replay

## Architecture

### Backend (C++)
- HTTP server using cpp-httplib
- Custom .omni file format for storage
- B-Tree indexing for users and snapshots
- Hash-based email lookup
- SHA-256 password hashing

### Frontend (HTML/CSS/JS)
- Canvas rendering with zoom/pan
- HTTP polling for updates
- Tile-based loading for performance

### Data Storage
- Single `canvas.omni` file
- In-memory B-Trees serialized to disk
- Fixed-size file structure
- No external database required

## API Endpoints

- `GET /` - Serve frontend
- `GET /canvas` - Get visible canvas tiles
- `POST /place_pixel` - Place a pixel
- `POST /register` - Register new user
- `POST /login` - User login
- `GET /chat` - Get chat messages
- `POST /chat` - Send chat message
- `GET /quests` - Get active quests
- `GET /season` - Get current season
- `GET /episode` - Get episode info
- `GET /export_png` - Download current canvas as PNG
- `GET /export_video` - Generate and download video replay
- `GET /history` - Get previous episode thumbnails

## Configuration

Edit constants in `backend/main.cpp`:
- `PORT` - Server port (default: 8080)
- `CANVAS_SIZE` - Canvas dimensions (default: 50×50)
- `EPISODE_DURATION` - Episode length in seconds (default: 900 = 15 min)
- `SNAPSHOT_INTERVAL` - Snapshot frequency (default: 10 seconds)
- `USER_COOLDOWN` - Cooldown for registered users (default: 5 seconds)
- `GUEST_COOLDOWN` - Cooldown for guests (default: 10 seconds)

## License

MIT License - Educational prototype