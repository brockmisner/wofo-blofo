const socket = io();

socket.on("mqtt_message", (data) => {
    const topicParts = data.topic.split("/");
    const deviceId = topicParts[1];
    const payload = JSON.parse(data.payload);

    const statusElement = document.querySelector(`#${deviceId} .status`);
    if (statusElement) {
        statusElement.textContent = payload.status || "Unknown";
        statusElement.style.color = payload.status === "online" ? "green" : "red";
    }

    const logsElement = document.getElementById("logs");
    logsElement.innerHTML += `<p>${new Date().toLocaleTimeString()}: ${data.topic} - ${JSON.stringify(payload)}</p>`;
    logsElement.scrollTop = logsElement.scrollHeight;
});
