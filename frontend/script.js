const bulb = document.getElementById("bulb");
const stateEl = document.getElementById("state");
const updatedEl = document.getElementById("updated");
const conn = document.getElementById("conn");
const connText = document.getElementById("conn-text");

let ws = null;
let reconnectTimer = null;
let pollTimer = null;

function setConnection(status) {
    conn.classList.remove("online", "offline");
    if (status === "online") {
        conn.classList.add("online");
        connText.textContent = "Ulangan";
    } else if (status === "offline") {
        conn.classList.add("offline");
        connText.textContent = "Uzilgan";
    } else {
        connText.textContent = "Ulanmoqda...";
    }
}

function render(data) {
    if (!data) return;
    const state = (data.state || "LOW").toUpperCase();
    if (state === "HIGH") {
        bulb.classList.add("on");
        document.body.classList.add("lit");
        stateEl.textContent = "HIGH";
        stateEl.classList.add("high");
        stateEl.classList.remove("low");
    } else {
        bulb.classList.remove("on");
        document.body.classList.remove("lit");
        stateEl.textContent = "LOW";
        stateEl.classList.add("low");
        stateEl.classList.remove("high");
    }
    if (data.updated_at) {
        updatedEl.textContent = "Oxirgi: " + formatTime(data.updated_at);
    } else {
        updatedEl.textContent = "hech qachon";
    }
}

function formatTime(iso) {
    try {
        const d = new Date(iso);
        return d.toLocaleTimeString("uz-UZ", {
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
        });
    } catch {
        return iso;
    }
}

function wsUrl() {
    const proto = location.protocol === "https:" ? "wss:" : "ws:";
    return `${proto}//${location.host}/ws`;
}

function connect() {
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
    }
    setConnection("connecting");
    try {
        ws = new WebSocket(wsUrl());
    } catch (e) {
        scheduleReconnect();
        return;
    }

    ws.onopen = () => {
        setConnection("online");
        stopPolling();
    };

    ws.onmessage = (ev) => {
        try {
            render(JSON.parse(ev.data));
        } catch (e) {
            console.error("bad message", e);
        }
    };

    ws.onclose = () => {
        setConnection("offline");
        startPolling();
        scheduleReconnect();
    };

    ws.onerror = () => {
        try { ws.close(); } catch {}
    };
}

function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        connect();
    }, 2000);
}

async function pollOnce() {
    try {
        const res = await fetch("/status", { cache: "no-store" });
        if (res.ok) render(await res.json());
    } catch {}
}

function startPolling() {
    if (pollTimer) return;
    pollOnce();
    pollTimer = setInterval(pollOnce, 2000);
}

function stopPolling() {
    if (pollTimer) {
        clearInterval(pollTimer);
        pollTimer = null;
    }
}

pollOnce();
connect();

document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "visible") {
        if (!ws || ws.readyState !== WebSocket.OPEN) {
            connect();
        }
        pollOnce();
    }
});
