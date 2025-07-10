var updateTimeInnterval = 1000;//Todo ........
var visibleTab = "";
let ws;

// select the buttons with the class containng tabButton
let tabButtons = document.querySelectorAll('.tabButton');
if (tabButtons) {
    tabButtons.forEach(button => {
        button.addEventListener('click', function () {
            // Remove 'active' class from all buttons
            tabButtons.forEach(btn => btn.classList.remove('active'));
            // Add 'active' class to the clicked button
            this.classList.add('active');
            // Get the target tab from the data attribute
            let targetTab = this.getAttribute('data-target');
            // Hide all tabs
            let tabs = document.querySelectorAll('.tab');
            tabs.forEach(tab => {
                tab.classList.remove('activeTab')
            });
            // Show the target tab
            let target = document.getElementById(targetTab);
            if (target) {
                target.classList.add('activeTab');
                visibleTab = targetTab;
            }
            // create a custom event to notify the main process
            let event = new CustomEvent('tabChanged', {
                detail: {
                    target: targetTab,
                    id: this.id
                }
            });
            // Dispatch the event
            document.dispatchEvent(event);
        });
    });
}

// Select the home automatically active tab
let activeTab = document.querySelector('.tabButton.active');
if (!activeTab) {
    let homeTab = document.querySelector('.tabButton[data-target="home"]');
    if (homeTab) {
        homeTab.classList.add('active');
        let homeContent = document.getElementById('home');
        if (homeContent) {
            homeContent.classList.add('activeTab');
            visibleTab = "home";
            setSystemInfo();
            setWifiInfo();
        }
    }
}

let property = "";
let value = "";
let sysContainer = document.getElementById("sysInfo");

let systemInfoContainer = `<div class="d-flex align-items-center" >
                <p class="card-title m-0 h6 text-primary mx-1" >`+ property + ` :</p>
                <p class="card-text mx-1" >`+ value + `</p>
              </div>`

function setSystemInfo() {
    // Helper function to make keys more readable
    function formatKey(key) {
        return key.replace(/_/g, ' ')
            .replace(/\b\w/g, c => c.toUpperCase());
    }

    // If websocket is connected, request system info via ws
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ get: "sinf" }));

        // Listen for a single response for system info
        function handleSystemInfo(event) {
            try {
                const data = JSON.parse(event.data);

                if (data && typeof data === "object") {
                    sysContainer.innerHTML = '';
                    for (let key in data) {
                        let displayValue = data[key];
                        let unit = '';
                        switch (key) {
                            case 'uptime':
                                unit = 's';
                                break;
                            case 'cpu_freq_mhz':
                                unit = 'MHz';
                                break;
                            case 'temperature':
                                unit = '°C';
                                displayValue = parseFloat(displayValue).toFixed(1);
                                break;
                            case 'free_heap':
                            case 'total_heap':
                            case 'min_free_heap':
                            case 'psram_size':
                            case 'free_psram':
                            case 'flash_size':
                                unit = 'B';
                                break;
                            case 'cores':
                            case 'chip_revision':
                            case 'features':
                                unit = '';
                                break;
                        }
                        if (unit) displayValue += ' ' + unit;
                        let info = `<div class="d-flex align-items-center">
                            <p class="card-title m-0 h6 text-primary mx-1">${formatKey(key)} :</p>
                            <p class="card-text mx-1">${displayValue}</p>
                        </div>`;
                        sysContainer.innerHTML += info;
                    }
                    ws.removeEventListener('message', handleSystemInfo);
                }
            } catch (e) {
                // Ignore non-JSON or unrelated messages
            }
        }
        ws.addEventListener('message', handleSystemInfo);
    }
}

function setWifiInfo() {
    function formatKey(key) {
        return key.replace(/_/g, ' ')
            .replace(/\b\w/g, c => c.toUpperCase());
    }

    let wifiContainer = document.getElementById("wifiInfo");

    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ get: "ws" }));

        function handleWifiInfo(event) {
            try {
                const data = JSON.parse(event.data);
                if (data && typeof data === "object" && data.wifi_status) {
                    wifiContainer.innerHTML = '';
                    for (let key in data.wifi_status) {
                        let displayValue = data.wifi_status[key];
                        let unit = '';
                        switch (key) {
                            case 'rssi':
                                unit = 'dBm';
                                break;
                            case 'channel':
                                unit = '';
                                break;
                            // Add more cases as needed for wifi info
                        }
                        if (unit) displayValue += ' ' + unit;
                        let info = `<div class="d-flex align-items-center">
                            <p class="card-title m-0 h6 text-primary mx-1">${formatKey(key)} :</p>
                            <p class="card-text mx-1">${displayValue}</p>
                        </div>`;
                        wifiContainer.innerHTML += info;
                    }
                    ws.removeEventListener('message', handleWifiInfo);
                }
            } catch (e) {
                // Ignore non-JSON or unrelated messages
            }
        }
        ws.addEventListener('message', handleWifiInfo);
    }
}

// Template function
function template(strings, ...values) {
    return strings.reduce((result, str, i) => {
        return result + str + (values[i] !== undefined ? values[i] : "");
    }, "");
}

document.addEventListener('tabChanged', function (e) {
    if (e.detail && e.detail.target == 'home') {
        setSystemInfo();
        setWifiInfo();
    }
});

let tempDatapoints = [];
let freePsramDatapoints = [];
let freeHeapDatapoints = [];
let rssiDatapoints = [];
let rssiMaxPoints = 100;
let espSsid = "";
let espUptime = "";
let tempMaxPoints = 120;
let freePsramMaxPoints = 360;
let freeHeapMaxPoints = 360;
let tempUpdateRate = 2000;

var chartTemp = new CanvasJS.Chart("tempChart", {
    title: {
        text: "Uptime"
    },
    data: [{
        type: "line",
        dataPoints: tempDatapoints
    }]
});

var chartWifi = new CanvasJS.Chart("wifiChart", {
    title: {
        text: "Wifi"
    },
    data: [{
        type: "spline",
        dataPoints: rssiDatapoints
    }]
});

var chartMemory = new CanvasJS.Chart("memChart", {
    animationEnabled: true,
    title: {
        text: "Memory , Min freeHeap : "
    },
    toolTip: {
        shared: true
    },
    data: [{
        name: "Free Psram",
        type: "spline",
        yValueFormatString: "# kb",
        showInLegend: true,
        dataPoints: freePsramDatapoints
    },
    {
        name: "Free Heap",
        type: "spline",
        yValueFormatString: "# kb",
        showInLegend: true,
        dataPoints: freeHeapDatapoints
    }]
});

function updateCharts() {
    if (visibleTab == "home") {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ get: "ddata" }));

            function handleDynamicData(event) {
                try {
                    const data = JSON.parse(event.data);
                    if (data && data.dynamic_data) {
                        // Temperature
                        if (typeof data.dynamic_data.temperature !== 'undefined') {
                            if (tempDatapoints.length >= tempMaxPoints) {
                                tempDatapoints.shift();
                            }
                            tempDatapoints.push({
                                x: new Date(),
                                y: data.dynamic_data.temperature
                            });
                        }
                        // Free PSRAM
                        if (typeof data.dynamic_data.free_psram !== 'undefined') {
                            if (freePsramDatapoints.length >= freePsramMaxPoints) {
                                freePsramDatapoints.shift();
                            }
                            freePsramDatapoints.push({
                                x: new Date(),
                                y: data.dynamic_data.free_psram / 1024
                            });
                        }
                        // Free Heap
                        if (typeof data.dynamic_data.free_heap !== 'undefined') {
                            if (freeHeapDatapoints.length >= freeHeapMaxPoints) {
                                freeHeapDatapoints.shift();
                            }
                            freeHeapDatapoints.push({
                                x: new Date(),
                                y: data.dynamic_data.free_heap / 1024
                            });
                        }
                        // Chart title
                        if (typeof chartMemory.title !== 'undefined') {
                            let strt = "Memory ";
                            if (typeof data.dynamic_data.min_free_heap !== 'undefined') {
                                strt += "Min freeHeap : " + parseInt(data.dynamic_data.min_free_heap / 1024) + " kb ";
                            }
                            if (typeof data.dynamic_data.free_heap !== 'undefined') {
                                strt += "Ram : " + parseInt((data.dynamic_data.free_heap - data.dynamic_data.free_psram) / 1024) + " kb ";
                            }
                            chartMemory.title.set("text", strt);
                        }

                        if (typeof chartWifi.title !== 'undefined') {
                            chartWifi.title.set("text", `WiFi: ` + espSsid);
                        }

                        if (typeof chartTemp.title !== 'undefined') {
                            chartTemp.title.set("text", espUptime);
                        }
                        // wifi dynamic data
                        // WiFi dynamic data: show RSSI as signal strength, SSID as chart title
                        if (typeof data.dynamic_data.ssid !== 'undefined') {
                            if (document.getElementById("wifiContainer")) document.getElementById("wifiContainer").classList.remove("d-none");
                            if (document.getElementById("wifiInfoContainer")) document.getElementById("wifiInfoContainer").classList.remove("d-none");

                            // Convert RSSI to signal strength percentage (approximate)
                            // RSSI: -100 dBm (worst) to -30 dBm (best)
                            espSsid = data.dynamic_data.ssid;
                            let rssi = data.dynamic_data.rssi;
                            let strength = Math.max(0, Math.min(100, Math.round(2 * (rssi + 100))));
                            if (rssiDatapoints.length >= rssiMaxPoints) {
                                rssiDatapoints.shift();
                            }
                            rssiDatapoints.push({
                                x: new Date(),
                                y: strength
                            });

                            chartWifi.render();
                        } else {
                            if (document.getElementById("wifiContainer")) document.getElementById("wifiContainer").classList.add("d-none");
                            if (document.getElementById("wifiInfoContainer")) document.getElementById("wifiInfoContainer").classList.add("d-none");

                        }

                        if (typeof data.dynamic_data.uptime !== 'undefined') {
                            // Convert uptime from seconds to human-readable format
                            let seconds = data.dynamic_data.uptime;
                            let uptimeStr = "";
                            if (seconds < 60) {
                                uptimeStr = `${seconds} s`;
                            } else if (seconds < 3600) {
                                uptimeStr = `${Math.floor(seconds / 60)} min ${seconds % 60} s`;
                            } else if (seconds < 86400) {
                                let hours = Math.floor(seconds / 3600);
                                let mins = Math.floor((seconds % 3600) / 60);
                                uptimeStr = `${hours} h ${mins} min`;
                            } else {
                                let days = Math.floor(seconds / 86400);
                                let hours = Math.floor((seconds % 86400) / 3600);
                                uptimeStr = `${days} d ${hours} h`;
                            }
                            espUptime = "Uptime : " + uptimeStr;
                        }

                        chartTemp.render();
                        chartMemory.render();



                        ws.removeEventListener('message', handleDynamicData);
                    }
                } catch (e) {
                    // Ignore non-JSON or unrelated messages
                }
            }
            ws.addEventListener('message', handleDynamicData);
        }
    }
}

function setupCharts() {
    setInterval(function () { updateCharts() }, 2000);
}

window.onload = function () {
    setupCharts();
};

// Connect to the ESP32 WebSocket server


function connectWebSocket() {
    // Replace with the actual IP or use hostname for dynamic setups
    const host = window.location.hostname;
    const wsURL = `ws://${host}/ws`;
    ws = new WebSocket(wsURL);

    ws.onopen = function () {
        console.log('[WS] Connected');
        setSystemInfo();
        setWifiInfo();
        updateCharts();
    };

    ws.onmessage = function (event) {
    };

    ws.onclose = function (event) {
        console.warn(`[WS] Disconnected (code=${event.code}). Reconnecting in 3s...`);
        setTimeout(connectWebSocket, 3000);  // Retry every 3 seconds
    };

    ws.onerror = function (err) {
        console.error('[WS] Error:', err);
        ws.close();  // Trigger reconnect on error
    };
}

/**
 * Registers a one-time WebSocket message handler for a specific key in the received JSON.
 * @param {string} key - The key to look for in the JSON message.
 * @param {function} callback - Function to call with the value of the key when received.
 */
function handleWebSocketMessage(key, callback) {
    function handler(event) {
        try {
            const data = JSON.parse(event.data);
            if (data && typeof data === "object" && key in data) {
                callback(data[key], data); // Pass the value and the whole object
                ws.removeEventListener('message', handler);
            }
        } catch (e) {
            // Ignore non-JSON or unrelated messages
        }
    }
    ws.addEventListener('message', handler);
}

// Call this on page load
window.addEventListener('load', connectWebSocket);