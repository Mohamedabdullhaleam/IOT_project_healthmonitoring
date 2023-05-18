const express = require('express');
const path = require('path');
const { WebSocketServer } = require('ws');
const fs = require('fs');

const app = express();
const port = 3000;

const server = app.listen(port, function() {
  console.log("Server is running at port" + port);
});

app.get('/', function(req, res) {
  const filePath = path.resolve('D:/iot2/SensorsTest - Copy.html');
  res.sendFile(filePath);
});

const wss = new WebSocketServer({ server });

let lastRecordedTime = null; 

wss.on('connection', (ws) => {
  console.log('someone connected');
  
  ws.on('message', (msg) => {
    Broadcast(msg.toString('utf8'));
    const txtPath = 'D:/iot2/New Text Document.txt';
    const data = JSON.parse(msg.toString('utf8'));
    const now = new Date();
    const formattedDate = now.toISOString().slice(0, 10); 
    const formattedTime = now.toISOString().slice(11, 19); 
    
    if (formattedTime !== lastRecordedTime) { 
      const formattedData = `${formattedDate},${formattedTime},${data.value1},${data.value2}\n`;
      fs.appendFile(txtPath, formattedData, (err) => {
        if (err) throw err;
        // console.log('Data written to file');
      });
      lastRecordedTime = formattedTime;
    }
  });
});

function Broadcast(msg) {
  wss.clients.forEach(function(client) {
    if (client.readyState == client.OPEN) {
      client.send(msg);
    }
  });
}