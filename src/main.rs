mod fs;
mod pg;
mod diff;

extern crate clap;

use clap::{App, SubCommand};

fn main() {
    let matches = App::new("Postgres Schema Migrator")
        .version("0.1")
        .author("https://github.com/jwdeitch/pg_migrate")
        .subcommand(SubCommand::with_name("up")
            .about("forward migration, files postfixed with -up"))
        .subcommand(SubCommand::with_name("down")
            .about("rollback migrations, files postfixed with -down"))
        .subcommand(SubCommand::with_name("status")
            .about("Retrieve migration Status"))
        .subcommand(SubCommand::with_name("configure")
            .about("Setup migrations table"))
        .get_matches();

    if matches.is_present("status") {
        let test = diff::discover_mismatch(fs::walk(".".to_string(), true), pg::status());
        let test1 = diff::discover_mismatch(pg::status(), fs::walk(".".to_string(), true));
        println!("{:?}, {:?}", test, test1);
        pg::status();
    }

    if matches.is_present("up") {
        fs::walk(".".to_string(), true);
    }

    if matches.is_present("down") {
        fs::walk(".".to_string(), false);
    }

    if matches.is_present("configure") {
        pg::provision();
        println!("Configured! Try to `pg_migrate up/down`");
    }
}