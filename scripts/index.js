const WebSocketClient = require('websocket').client;

var client = new WebSocketClient();

client.on('connectFailed', function(error) {
    console.log('Connect Error: ' + error.toString());
});
 
client.on('connect', function(connection) {
    console.log('WebSocket Client Connected');
    connection.on('error', function(error) {
        console.log("Connection Error: " + error.toString());
    });
    connection.on('close', function() {
        console.log('echo-protocol Connection Closed');
    });
    connection.on('message', function(message) {
        if (message.type === 'utf8') {
            console.log("Received: '" + message.utf8Data + "'");
        }
    });
    
    function sendReinit() {
        if (connection.connected) {
            //connection.sendUTF("BEHOLDhttps://cdn.discordapp.com/attachments/249321466481475585/731313240818188368/Star_Trek_2020-07-10-18-56-27.jpg");
            connection.sendUTF("VOYIMAGEhttps://cdn.discordapp.com/attachments/296001137809686528/732071634352996373/image0.png");
        }
    }
    sendReinit();
});

client.connect('ws://localhost:5001/');
