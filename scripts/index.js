const WebSocket = require('ws');

async function connectWebSocket(url) {
	return new Promise((resolve, reject) => {
		const ws = new WebSocket(url);

		ws.on('open', function open() {
			resolve(ws);
		});
	});
}

async function sendAndWait(ws, message) {
	return new Promise((resolve, reject) => {
		ws.on('message', function incoming(data) {
			resolve(data);
		});
		ws.send(message);
	});
}

connectWebSocket('ws://localhost:5001/').then(async ws => {
    let reply = await sendAndWait(ws, 'BOTHhttps://cdn.discordapp.com/attachments/296001137809686528/732071634352996373/image0.png');
    console.log(reply);

    reply = await sendAndWait(ws, 'BOTHhttps://cdn.discordapp.com/attachments/296001137809686528/732071634352996373/image0.png');
    console.log(reply);
})