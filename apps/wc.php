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
    public ?DateTime $date_of_last_logon,
    public ?DateTime $created_at,
    public ?DateTime $updated_at,
    public int $email_failure_count,
    public bool $invalid,
    public ?DateTime $invalidated_time
  ) {
  }

  function __toString(): string
  {
    return $this->firstname . " " . $this->lastname . " <" . $this->email . ">\n" .
      "dob=" . format_optional_date("Y-m-d", $this->dob) . "\n" .
      "date_of_last_logon=" . format_optional_date("Y-m-d H:i:s", $this->date_of_last_logon) . "\n" .
      "created_at=" . format_optional_date("Y-m-d H:i:s", $this->created_at) . "\n" .
      "updated_at=" . format_optional_date("Y-m-d H:i:s", $this->updated_at) . "\n" .
      "email_failure_cout=" . $this->email_failure_count . "\n" .
      "invalid=" . format_bool($this->invalid) . "\n" .
      "invalidated_time=" . format_optional_date("Y-m-d H:i:s", $this->invalidated_time) . "\n";
  }
}

function db(): PDO
{
  static $con;
  if (!$con instanceof PDO) {
    $ini = (object)parse_ini_file(dirname($_SERVER['argv'][0]) . DIRECTORY_SEPARATOR . "myslice_demo.ini");
    $con = new PDO(
      'mysql:dbname=' . $ini->db_db . ';' .
        'host=' . ($ini->db_host ?? 'localhost') . ';' .
        'charset=utf8',
      $ini->db_user,
      $ini->db_pass
    );
    $con->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY, false);
  }
  return $con;
}

function load_members(int $limit = 10000): array  // of Member
{
  $members = [];
  foreach (db()->query("select " .
    "  firstname, " .
    "  lastname, " .
    "  email, " .
    "  dob, " .
    "  date_of_last_logon, " .
    "  created_at, " .
    "  updated_at, " .
    "  email_failure_count, " .
    "  invalid, " .
    "  invalidated_time " .
    "from member limit " . $limit, PDO::FETCH_NUM) as $row) {
    $members[] = new Member(
      $row[0],
      $row[1],
      $row[2],
      parse_optional_date("Y-m-d", $row[3]),
      parse_optional_date("Y-m-d H:i:s", $row[4]),
      parse_optional_date("Y-m-d H:i:s", $row[5]),
      parse_optional_date("Y-m-d H:i:s", $row[6]),
      (int)$row[7],
      (bool)$row[8],
      parse_optional_date("Y-m-d H:i:s", $row[9])
    );
  }
  return $members;
}

function report_topn(int $N, array $freqs): void
{
  arsort($freqs);
  foreach (array_slice($freqs, 0, $N) as $key => $freq)
    printf("%-20s %6d\n", $key, $freq);

  printf("%-20s %6d\n", "Total", array_sum($freqs));
}

function report_stats(int $N, array $members): void
{
  $failed = [];
  $invalid = [];
  foreach ($members as $member) {
    $domain = substr($member->email, strpos($member->email, "@") + 1);

    if ($domain != "temp.webcollect.org.uk") {
      if ($member->invalid) {
        if (!isset($invalid[$domain])) $invalid[$domain] = 0;
        $invalid[$domain]++;
      }

      if (!isset($failed[$domain])) $failed[$domain] = 0;
      $failed[$domain] += $member->email_failure_count;
    }
  }

  echo "invalid\n";
  report_topn($N, $invalid);

  echo "\nfailed\n";
  report_topn($N, $failed);
}

function dump_members(array $members): void
{
  foreach ($members as $m) echo $m . "\n";
}

function timer(string $label, callable $work): mixed
{
  $start = microtime(true);
  $retval  = $work();
  $end = microtime(true);
  fprintf(STDERR, "%-20s %12.4f ms\n", $label, ($end - $start) * 1000);
  return $retval;
}

// main

try {
  $members = timer("load", function () {
    return load_members((int)($_SERVER['argv'][1] ?? 10000));
  });
  timer("stats", function () use ($members) {
    report_stats((int)($_SERVER['argv'][2] ?? 10), $members);
  });
  timer("dump", function () use ($members) {
    dump_members($members);
  });
} catch (Exception $e) {
  fwrite(STDERR, "Something went wrong: Execption: " . $e->getMessage() . "\n");
  exit(1); // failure
}
