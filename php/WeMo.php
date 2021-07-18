<?php

/**
 *  @file   WeMo.cpp
 *  @brief  WeMo Class
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

 require_once('Discover.php');

class WeMo {

    private $discovery;

    public $timers = Array();

    public function __construct() {

        if(file_exists('wemo.ini')) {

            $ini = parse_ini_file('wemo.ini', true);
        }

        $this->discovery = new Discover();

        foreach($this->discovery->plugs as &$plug) {

            $name = $plug->Name();

            if(array_key_exists($name, $ini)) {

                $cfg = $ini[$name];

                if(array_key_exists('daily', $cfg)) {

                    if($cfg['daily']) {

                        if(array_key_exists('ontimes', $cfg)) {

                            foreach(explode(',', $cfg['ontimes']) as $time) {

                                $t = explode(':', $time);

                                $this->timers['daily'][] = Array('plug' => &$plug,
                                                                 'time' => intval($t[0]) * 3600 + intval($t[1]) * 60,
                                                                 'action' => 'on');

                            }
                        }

                        if(array_key_exists('offtimes', $cfg)) {

                            foreach(explode(',', $cfg['offtimes']) as $time) {

                                $t = explode(':', $time);

                                $this->timers['daily'][] = Array('plug' => &$plug,
                                                                 'time' => intval($t[0]) * 3600 + intval($t[1]) * 60,
                                                                 'action' => 'off');
                            }
                        }
                    }
                }
            }
        }
    }
}

//$wemo = new WeMo();
