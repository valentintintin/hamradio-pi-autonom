drop table if exists logs;

drop table if exists sensors;

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
    nightDetected      INTEGER,
    alertAsserted      INTEGER,
    rawMpptchg         text
);

