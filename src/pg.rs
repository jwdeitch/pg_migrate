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


pub fn status() {
    let conn_ptr = load();

    let is_provisioned = conn_ptr.execute(r#"
            SELECT case when count(*) > 0 then true else false end FROM pg_tables where schemaname = 'public' and tablename = 'pg_migrate';
        "#, &[]);

    println!("{:?}", is_provisioned)

//    if is_provisioned.unwrap() {
//        "Looks like things haven't been setup yet - try `pg_migrate provision`".to_string()
//    }
}

pub fn provision() {
    let success = load().execute(r#"
    with i as (
        CREATE TYPE pg_migrate_direction AS ENUM ('up', 'down')
    )
    CREATE TABLE pg_migrate (
        filename       VARCHAR,
        direction      pg_migrate_direction,
        batch          INT,
        time_performed TIMESTAMP DEFAULT now()
    );
        "#, &[]);

    match success {
        Ok(v) => return,
        Err(e) => println!("Something went wrong: {:?}", e),
    }


}