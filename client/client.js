const net = require('net');

const HOST = '127.0.0.1';
const PORT = 25;

const client = net.createConnection({ host: HOST, port: PORT }, () => {
  console.log('Connected to SMTP server');
});

client.on('data', (data) => {
  console.log(data.toString());
});

client.on('end', () => {
  console.log('Disconnected from SMTP server');
});

client.write('HELO example.com\r\n');
client.write('MAIL FROM:<sender@example.com>\r\n');
client.write('RCPT TO:<recipient@example.com>\r\n');
client.write('DATA\r\n');
client.write('Subject: Test email\r\n');
client.write('\r\n');
client.write('This is a test email.\r\n');
client.write('.\r\n');
client.write('QUIT\r\n');
//写的有问题，待定等着改