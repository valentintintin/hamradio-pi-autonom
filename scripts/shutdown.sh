#!/bin/bash

date

curl -f http://localhost/states/shutdown && echo "Will shutdown !" && /sbin/halt
