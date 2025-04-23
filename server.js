const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const express = require('express');
const app = express();
const PORT = 3000;

// === Serial Port Setup ===
const port = new SerialPort({ path: 'COM7', baudRate: 9600 });
const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

// === Serve Static Files (index.html, pdf-viewer.html, doc folder, etc.) ===
app.use(express.static('public'));

// === Map Username to PDF Document Path ===
const userDocs = {
    Janice: 'doc/Janice/Janice.pdf',
    Bronil: 'doc/Bronil/Bronil.pdf'
};

// === Store Latest URL for Frontend Polling ===
let latestUrl = '';

// === Handle Incoming Serial Data from Arduino ===
parser.on('data', (line) => {
    line = line.trim();
    console.log('[From Arduino]', line);

    // Handle: SEND:Username
    if (line.startsWith('SEND:')) {
        const uname = line.split(':')[1].trim();
        const pdfPath = userDocs[uname];

        if (pdfPath) {
            // 👇 Correctly set the URL for frontend
            latestUrl = `/pdf-viewer.html?pdf=${encodeURIComponent(pdfPath)}`;

            // 👇 Send INDEX PAGE only to Arduino
            const response = `URL:/index.html\n`;
            console.log('[To Arduino]', response.trim());
            port.write(response);
        } else {
            // 👇 Invalid user fallback
            latestUrl = '';
            port.write('URL:not-authorized\n');
        }
    }
});



// === CLI Command to Trigger Scan ===
process.stdin.setEncoding('utf8');
process.stdin.on('data', (input) => {
    const command = input.trim();
    if (command.toLowerCase() === 'ready') {
        port.write('ready\n');
        console.log('[Sent to Arduino] ready');
    }
});

// === Frontend Polling Endpoint ===
app.get('/latest', (req, res) => {
    res.send({ url: latestUrl });
});

// === Start Express Server ===
app.listen(PORT, () => {
    console.log(`🚀 Server running at http://localhost:${PORT}`);
    console.log('🖐️ Type "ready" in this terminal to trigger fingerprint scan.');
});
