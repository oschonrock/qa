<?php

class Member
{
  public function __construct(
    public string $firstname,
    public string $lastname,
    public string $email
  ) {
  }

  function __toString()
  {
    return $this->firstname . " " . $this->lastname . " <" . $this->email . ">";
  }
}

$ini = (object)parse_ini_file("myslice_demo.ini");
$con = new PDO(
  'mysql:dbname=' . $ini->db_db .
    ';host=' . $ini->db_host,
  $ini->db_user,
  $ini->db_pass
);

$members = [];
foreach ($con->query("select firstname, lastname, email from member", PDO::FETCH_NUM) as $row)
  $members[] = new Member(...$row);

foreach ($members as $m) echo $m . "\n";
