import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Set

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

app = FastAPI(title="Power Monitor")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

BASE_DIR = Path(__file__).resolve().parent
FRONTEND_DIR = Path(os.environ.get("FRONTEND_DIR", BASE_DIR.parent / "frontend"))
STATE_FILE = Path(os.environ.get("STATE_FILE", BASE_DIR / "state.json"))


class StatusUpdate(BaseModel):
    state: str


def load_state() -> dict:
    if STATE_FILE.exists():
        try:
            return json.loads(STATE_FILE.read_text())
        except Exception:
            pass
    return {"state": "LOW", "updated_at": None}


def save_state(data: dict) -> None:
    try:
        STATE_FILE.write_text(json.dumps(data))
    except Exception:
        pass


current = load_state()
clients: Set[WebSocket] = set()


def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


async def broadcast():
    if not clients:
        return
    dead = []
    for ws in clients:
        try:
            await ws.send_json(current)
        except Exception:
            dead.append(ws)
    for ws in dead:
        clients.discard(ws)


@app.post("/status")
async def update_status(payload: StatusUpdate):
    state = payload.state.strip().upper()
    if state not in ("HIGH", "LOW"):
        raise HTTPException(status_code=400, detail="state must be HIGH or LOW")
    current["state"] = state
    current["updated_at"] = now_iso()
    save_state(current)
    await broadcast()
    return {"ok": True, **current}


@app.get("/status")
async def get_status():
    return current


@app.get("/health")
async def health():
    return {"ok": True}


@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    clients.add(ws)
    try:
        await ws.send_json(current)
        while True:
            await ws.receive_text()
    except WebSocketDisconnect:
        pass
    finally:
        clients.discard(ws)


app.mount("/", StaticFiles(directory=FRONTEND_DIR, html=True), name="frontend")


if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 8000))
    uvicorn.run("main:app", host="0.0.0.0", port=port, reload=False)
