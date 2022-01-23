<?php

/**
 *  @file   Discover.php
 *  @brief  Discover Class
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/


require_once('Plug.php');

class Discover {

  const address = "239.255.255.250";

  const port = 1900;

  public $plugs = Array();

  private $sock;

  public function __construct() {

    $this->sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);

    socket_set_option($this->sock, SOL_SOCKET, SO_REUSEADDR, 1);

    $grp = Array("group" => self::address,
                 "interface" => 0);

    socket_set_option($this->sock, IPPROTO_IP, MCAST_JOIN_GROUP, $grp);

    $this->broadcast();

    $this->receive();

    socket_set_option($this->sock, IPPROTO_IP, MCAST_LEAVE_GROUP, $grp);

    socket_close($this->sock);
  }

  private function broadcast() {

    $msg = Array('M-SEARCH * HTTP/1.1',
                 'HOST: ' . self::address . ':' . self::port,
                 'MAN: "ssdp:discover"',
                 'MX: 2',
                 //'ST: ssdp:all'
                 'ST: urn:Belkin:device:controllee:1'
    );

    $pkt = implode("\r\n", $msg) . "\r\n\r\n";

    socket_sendto($this->sock, $pkt, strlen($pkt), 0, self::address, self::port);
  }

  private function receive() {

    $port = self::port;

    $buf = "";

    $len = 2048;

    $flags = 0;

    $name = "";

    $grp = Array("group" => self::address,
                 "interface" => 0);

    socket_set_option($this->sock, SOL_SOCKET, SO_RCVTIMEO, Array("sec" => 3,
                                                                  "usec" => 0));

    $timeout = time() + 3;

    while(time() < $timeout) {

      if(($bytes = socket_recvfrom($this->sock, $buf, $len, $flags, $name, $port)) === false) {

        $errno = socket_last_error($this->sock);
        if(!($errno == SOCKET_EAGAIN || $errno == SOCKET_EWOULDBLOCK))
           echo socket_strerror($errno);
        socket_clear_error($this->sock);
        continue;
      }

      if($bytes > 0) {

        $headers = http_parse_headers($buf);

        if(array_key_exists('LOCATION', $headers)) {

          $parts = parse_url($headers['LOCATION']);

          $this->plugs[] = new Plug($parts['host'], intval($parts['port']));
        }
      }
    }
  }
}

function http_parse_headers($header) {
  $headers = Array();
  foreach(explode("\r\n", $header) as $h) {
    $kv = explode(': ', $h);
    if(count($kv) > 1)
      $headers[trim($kv[0])] = trim($kv[1]);
  }
  return $headers;
}

//$discover = new Discover();
//$i = 0;
//foreach($discover->plugs as $p) {

  //while($i < 10) {
//  echo ($i + 1) . ': ' . $p->Name() . "\n";
  //$p->Toggle();
  //sleep(1);
  // $p->On();
  // sleep(1);
//  ++$i;
  //}
//}
