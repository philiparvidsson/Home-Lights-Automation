//! # Light Switch Manager
//!
//! Sends commands to the light switch manager.
//!
//! **Usage:** lm <on/off> <group> <socket>
//!
//! **Example:** lm on a 1

use std::io::Write;
use std::net::TcpStream;

/// Main function.
fn main() {
    let argv: Vec<String> = std::env::args().collect();

    // Build command string from command line arguments.
    let cmd = argv[1..].join(" ");

    if cmd.len() == 0 {
        println!("no command to send");
        return;
    }

    // Attempt to connect to the light manager.
    match TcpStream::connect("192.168.1.7:23") {
        Ok(ref mut conn) => send_command(&cmd, conn),

        _ => println!("could not connect")
    };
}

/// Sends a command through the specified connection.
fn send_command(cmd: &String, conn: &mut TcpStream) {
    let data = cmd.clone() + "\n";
    let _ = conn.write(data.as_bytes());
}
