<?php

function parse_optional_date(string $format, ?string $str): ?DateTime
{
  if ($str === null) return null;
  $d = DateTime::createFromFormat($format, $str);
  if ($d === false)
    throw new Exception('couldn\'t parse date `' . $str . '` with format `' . $format . '`');
  return $d;
}

function format_optional_date(string $format, ?DateTime $date): string
{
  if ($date === null) return "";
  return $date->format($format);
}

function format_bool(bool $b): string
{
  return ($b ? "true" : "false");
}

class Member
{
  public function __construct( // new php8.0 syntax: auto create members and fill them
    public string $firstname,
    public string $lastname,
    public string $email,
    public ?DateTime $dob,
    public bool $invalid,
    public ?DateTime $invalidated_time
  ) {
  }

  function __toString(): string
  {
    return $this->firstname . " " . $this->lastname . " <" . $this->email . ">\n" .
      "dob=" . format_optional_date("Y-m-d", $this->dob) . "\n" .
      "invalid=" . format_bool($this->invalid) . "\n" .
      "invalidated_time=" . format_optional_date("Y-m-d H:i:s", $this->invalidated_time) . "\n";
  }
}

try {
  $ini = (object)parse_ini_file(dirname($argv[0]) . DIRECTORY_SEPARATOR . "myslice_demo.ini");
  $con = new PDO('mysql:dbname=' . $ini->db_db . ';' . 
                 'host=' . ($ini->db_host ?? 'localhost') . ';' . 
                 'charset=utf8',
                 $ini->db_user,
                 $ini->db_pass);
  
  $con->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY, false);

  $members = [];
  foreach ($con->query("select firstname, lastname, email, dob, invalid, invalidated_time " . 
                       "from member", PDO::FETCH_NUM) as $row) {
    $members[] = new Member(
      $row[0],
      $row[1],
      $row[2],
      parse_optional_date("Y-m-d", $row[3]),
      (bool)$row[4],
      parse_optional_date("Y-m-d H:i:s", $row[5])
    );
  }

  foreach ($members as $m) echo $m . "\n";
  
} catch (Exception $e) {
  fwrite(STDERR, "Something went wrong: Execption: " . $e->getMessage() . "\n");
  exit(1); // failure
}
