const express = require('express');
const path = require('path');
const  {WebSocketServer}  = require('ws');
 const app = express()

 const port = 3000


 const server = app.listen(port,function(){
    console.log("Server is runinng at port "+port);

 });
 app.get('/',function(req,res){
    const filePath = path.resolve('D:/iot2/SensorsTest.html');
    res.sendFile(filePath);  
    });
     const wss = new WebSocketServer({ server });

    wss.on('connection', (ws) =>  {
        console.log('someone connected');
        ws.on('message',(msg) => {
             Broadcast(msg.toString('utf8')) ;
         });
        
        
      }); 

function Broadcast(msg){
    wss.clients.forEach(function(client){
        if(client.readyState==client.OPEN){
            client.send(msg.toString('utf8'))
        }
    })
}
      

