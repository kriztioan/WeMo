<?php

/**
 *  @file   Plug.cpp
 *  @brief  Plug Class
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

 class Plug {

  const OFF = 0;
  const ON = 1;

  private $ip = "";
  private $port = "";

  public function __construct($ip, $port) {

    $this->ip = $ip;

    $this->port = $port;
  }

  public function Name($name = "") {

    if(!empty($name)) {

      return $this->SOAPRequest('SetFriendlyName', $name);
    }

    return $this->SOAPRequest('GetFriendlyName');
  }

  public function isOn() {

    return $this->State() == self::ON;
  }

  public function isOff() {

    return $this->State() == self::OFF;
  }

  public function Toggle() {

    if($this->isOn()) {

      return $this->Off();
    }

    return $this->On();
  }

  public function On() {

    if($this->isOff()) {

      return $this->SOAPRequest('SetBinaryState', self::ON) == '1';
    }

    return true;
  }

  public function Off() {

    if($this->isOn()) {

      return $this->SOAPRequest('SetBinaryState', self::OFF) == '0';
    }

    return true;
  }

  private function State() {

    return $this->SOAPRequest('GetBinaryState') == '1';
  }

  private function SOAPRequest($service, $arg = null) {

    if(!in_array($service, Array('GetFriendlyName', 'GetBinaryState', 'SetBinaryState'))) {

      return null;
    }

    $soap = new DOMDocument("1.0", "utf-8");
    //$soap->formatOutput = true;
    $soap->preserveWhiteSpace = false;
    $envelope = $soap->createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "s:Envelope");
    $envelope->setAttribute("s:encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    $body = $soap->createElement("s:Body");
    $serviceNode = $soap->createElementNS("urn:Belkin:service:basicevent:1", "u:" . $service);

    switch ($service) {

      case 'GetFriendlyName' :

        $serviceArg = null;
        break;

      case 'SetFriendlyName' :

        $serviceArg = $soap->createElement("FriendlyName", $arg);
        break;

      case 'GetBinaryState' :

        $serviceArg = null;
        break;

      case 'SetBinaryState' :

        $serviceArg = $soap->createElement("BinaryState", $arg);
        break;

      default:
        break;
    };

    $soap->appendChild($envelope);
    $envelope->appendChild($body);
    $body->appendChild($serviceNode);
    if(!is_null($serviceArg)) {
      $serviceNode->appendChild($serviceArg);
    }

    $request = $soap->saveXML(null, LIBXML_NOEMPTYTAG);

    $url = 'http://' . $this->ip . ':' . $this->port . '/upnp/control/basicevent1';

    $header = Array('Content-Type: text/xml; charset="utf-8"',
                    'Content-Length: ' . strlen($request),
                    'Accept: ',
                    'SOAPACTION: "urn:Belkin:service:basicevent:1#' . $service . '"');

    $opts = Array('http' => Array(
      'method' => 'POST',
      'header' => implode("\r\n", $header),
      'content' => $request)
    );

    //$ctx = stream_context_create($opts);

    //$xml = file_get_contents($url, false, $ctx);

    $ch = curl_init();

    curl_setopt($ch, CURLOPT_HTTPHEADER, $header); 

    curl_setopt($ch, CURLOPT_URL, $url);

    curl_setopt( $ch, CURLOPT_POSTFIELDS, $request);

    curl_setopt($ch, CURLOPT_POST, true);

    curl_setopt($ch, CURLOPT_HEADER, false);

    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

    curl_setopt($ch, CURLINFO_HEADER_OUT, true);

    $xml = curl_exec($ch);

    curl_close($ch);

    if($xml === false) {

      return null;
    }
    $soap->loadXML($xml);

    $serviceArg = $soap->getElementsByTagNameNS("urn:Belkin:service:basicevent:1", $service . 'Response');
    if($serviceArg->length == 1) {

      return $serviceArg->item(0)->nodeValue;
    }

    return null;
  }
}
