extern crate postgres;
extern crate dotenv;

use std::env;
use self::postgres::{Connection, TlsMode};

pub fn load() -> Box<Connection> {
    dotenv::dotenv().ok();

    let pg_url: String = env::var("pg_url").expect("pg_url .env var").to_string();
    println!("pg_url {}", pg_url);

    let connection = Connection::connect(pg_url, TlsMode::None).unwrap();
    Box::new(connection)
}


pub fn status() -> Vec<String> {
    let conn_ptr = load();

    let is_provisioned = conn_ptr.query(r#"
            SELECT filename FROM pg_migrate order by batch desc;
        "#, &[]).ok().expect("Looks like things haven't been setup yet - try `pg_migrate provision`");

    let num_of_migrations = is_provisioned.len();

    let mut latest_migrations: Vec<String> = Vec::new();
    if num_of_migrations > 0 {
        for row in &is_provisioned {
            latest_migrations.push(row.get(0))
        }
    }

    latest_migrations
}

pub fn provision() {
    let success = load().batch_execute(r#"
    CREATE TABLE pg_migrate (
        filename       VARCHAR,
        batch          INT,
        time_performed TIMESTAMP DEFAULT now()
    );
        "#);

    match success {
        Ok(v) => return,
        Err(e) => println!("Something went wrong: {:?}", e),
    }
}