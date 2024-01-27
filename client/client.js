// 引入nodemailer模块
const nodemailer = require('nodemailer');

// 定义SMTP服务器地址和端口
const SERVER_ADDRESS = 'localhost';
const SERVER_PORT = 587;

// 创建transporter对象，用于连接SMTP服务器
const transporter = nodemailer.createTransport({
  host: SERVER_ADDRESS,
  port: SERVER_PORT
});

// 定义邮件内容
const current_time = new Date().toUTCString();
const subject = 'Subject: Test email\r\n';
const from_addr = 'From: sender@example.com\r\n';
const to_addr = 'To: recipient@example.com\r\n';
const date = `Date: ${current_time}\r\n`;
const message_body = 'This is a test email.\r\n';
const message = subject + from_addr + to_addr + date + '\r\n' + message_body;

// 发送邮件
transporter.sendMail({
  from: 'sender@example.com',
  to: 'recipient@example.com',
  subject: 'Test email',
  text: message
}, (err, info) => {
  if (err) {
    console.error(err);
  } else {
    console.log(info);
  }
});
