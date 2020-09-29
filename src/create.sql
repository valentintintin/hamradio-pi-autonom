drop table if exists logs;
drop table if exists sensors;
drop table if exists variables;

create table logs
(
    id        integer
        primary key autoincrement,
    createdAt datetime    not null,
    service   varchar(32) not null,
    log       text        not null,
    data      text
);

create table sensors
(
    id                 integer
        primary key autoincrement,
    createdAt          datetime not null,
    voltageBattery     integer,
    voltageSolar       integer,
    currentBattery     integer,
    currentSolar       integer,
    currentCharge      integer,
    temperatureBattery integer,
    temperatureCpu     integer,
    temperatureRtc     integer,
    uptime             integer,
    rawMpptchg         text
);

create table variables
(
    id        integer
        primary key autoincrement,
    updatedAt datetime    not null,
    name      varchar(32) not null,
    data      text        not null
);
insert into variables(updatedAt, name, data)
values (1587229268000, 'seq_telemetry', 0);
insert into variables(updatedAt, name, data)
values (1587229268000, 'last_photo', '{}');
