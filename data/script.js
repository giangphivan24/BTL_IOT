var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var isAutoMode = true; 

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    try {
        initGauges();
    } catch(e) {
        console.warn("Không thể tải thư viện Gauge (do không có Internet), bỏ qua bước vẽ đồng hồ.");
    }
}

function onOpen(event) {
    console.log('Da ket noi WebSocket');
    document.getElementById('wsStatusDot').classList.add('connected');
    document.getElementById('wsStatusText').innerText = "Đã kết nối";
}

function onClose(event) {
    console.log('Mat ket noi WebSocket. Dang thu lai...');
    document.getElementById('wsStatusDot').classList.remove('connected');
    document.getElementById('wsStatusText').innerText = "Mất kết nối";
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
    } else {
        alert("WebSocket chua ket noi!");
    }
}

function onMessage(event) {
    try {
        var data = JSON.parse(event.data);
        
        if (data.type === "update") {
            // Cập nhật Đồng hồ (dữ liệu thật từ Cảm biến)
            if (gaugeTemp) gaugeTemp.refresh(data.temp);
            else document.getElementById("gauge_temp").innerHTML = `<h1 style='text-align:center; margin-top:40px; font-size:48px; color:#FF5722;'>${data.temp.toFixed(1)}°C</h1>`;
            
            if (gaugeHumi) gaugeHumi.refresh(data.humi);
            else document.getElementById("gauge_humi").innerHTML = `<h1 style='text-align:center; margin-top:40px; font-size:48px; color:#03A9F4;'>${data.humi.toFixed(1)}%</h1>`;

            // Cập nhật Trạng thái AI & Cảm biến
            updateStatusBadges(data.sys, data.ai);

            // Cập nhật Nút Auto/Manual
            isAutoMode = data.auto;
            const btnMode = document.getElementById('btnAutoMode');
            if (isAutoMode) {
                btnMode.className = "btn-mode mode-auto";
                btnMode.innerHTML = "AUTO (Tự động)";
            } else {
                btnMode.className = "btn-mode mode-manual";
                btnMode.innerHTML = "MANUAL (Thủ công)";
            }

            // Cập nhật Trạng thái Thiết bị (LED, Neo, Fan)
            updateDeviceButton('btnLed', data.led);
            updateDeviceButton('btnNeo', data.neo);
            updateDeviceButton('btnFan', data.fan);
        }
    } catch (e) {
        console.warn("JSON không hợp lệ:", event.data);
    }
}

var gaugeTemp, gaugeHumi;

function initGauges() {
    gaugeTemp = new JustGage({
        id: "gauge_temp", value: 0, min: -10, max: 60,
        donut: true, pointer: false, gaugeWidthScale: 0.25,
        levelColors: ["#00BCD4", "#4CAF50", "#FFC107", "#F44336"]
    });

    gaugeHumi = new JustGage({
        id: "gauge_humi", value: 0, min: 0, max: 100,
        donut: true, pointer: false, gaugeWidthScale: 0.25,
        levelColors: ["#42A5F5", "#00BCD4", "#0288D1"]
    });
}

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
}

function updateStatusBadges(sysState, aiState) {
    const sysEl = document.getElementById('sysStatus');
    if (sysState === 2) { sysEl.className = 'badge badge-critical'; sysEl.innerHTML = 'NGUY HIỂM'; }
    else if (sysState === 1) { sysEl.className = 'badge badge-warning'; sysEl.innerHTML = 'CẢNH BÁO'; }
    else { sysEl.className = 'badge badge-normal'; sysEl.innerHTML = 'BÌNH THƯỜNG'; }

    const aiEl = document.getElementById('aiStatus');
    if (aiState) { aiEl.className = 'badge badge-critical'; aiEl.innerHTML = 'PHÁT HIỆN BẤT THƯỜNG'; }
    else { aiEl.className = 'badge badge-normal'; aiEl.innerHTML = 'AN TOÀN'; }
}

function updateDeviceButton(btnId, state) {
    const btn = document.getElementById(btnId);
    
    // Đổi màu và chữ
    if (state) {
        btn.className = "toggle-btn on";
        btn.innerText = "ĐANG BẬT";
    } else {
        btn.className = "toggle-btn";
        btn.innerText = "ĐÃ TẮT";
    }

    // Nếu đang ở Auto -> Khóa mờ nút bấm
    if (isAutoMode) {
        btn.classList.add("disabled");
    } else {
        btn.classList.remove("disabled");
    }
}

// Gửi lệnh đổi Auto/Manual xuống ESP32
function toggleAuto() {
    Send_Data(JSON.stringify({
        page: "auto",
        value: { mode: !isAutoMode }
    }));
}

// Gửi lệnh Bật/Tắt thiết bị xuống ESP32 (Chỉ có tác dụng khi ở Manual)
function toggleDevice(devName) {
    if (isAutoMode) return;

    const btn = document.getElementById(
        devName === 'led' ? 'btnLed' : 
        devName === 'neo' ? 'btnNeo' : 'btnFan'
    );
    
    const currentState = btn.classList.contains("on");
    
    Send_Data(JSON.stringify({
        page: "device",
        value: {
            device: devName,
            status: !currentState
        }
    }));
}

document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();
    
    const payload = new URLSearchParams();
    payload.append("ssid", document.getElementById("ssid").value.trim());
    payload.append("pass", document.getElementById("password").value.trim());
    payload.append("token", document.getElementById("token").value.trim());
    payload.append("server", document.getElementById("server").value.trim());
    payload.append("port", document.getElementById("port").value.trim());

    fetch('/save', {
        method: 'POST',
        body: payload
    })
    .then(response => {
        if (response.ok) {
            alert("Da gui cau hinh! ESP32 dang luu vao Flash va khoi dong lai...");
        } else {
            alert("Loi: Khong the luu cau hinh.");
        }
    })
    .catch(error => {
        alert("Loi ket noi! Vui long kiem tra lai ket noi WiFi voi Yolo Uno.");
        console.error('Error:', error);
    });
});