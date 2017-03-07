//extern crate postgres;
extern crate dotenv;

use std::env;
//use postgres::{Connection, TlsMode};

pub fn load() {
    dotenv::dotenv().ok();

    let host: String;
    match env::var("host") {
        Ok(v) => host = v,
        Err(e) => panic!("missing .env var: host, {}", e),
    }

    println!("host {}", host)
}