<?php

require_once 'common.php';

header('Content-Type: application/json; charset=utf-8');

echo json_encode(getLastData());
