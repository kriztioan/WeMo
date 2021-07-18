#!/usr/bin/env /usr/bin/php
<?php

/**
 *  @file   wemod.php
 *  @brief  WeMo Mini Smart Plug Control Daemon
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 * 
 *  Run with:
 *    ./wemod.php &
 *
 ***********************************************/

date_default_timezone_set('America/Los_Angeles');

require_once('WeMo.php');

$wemo = new WeMo();


$signals = array(SIGALRM, SIGINT, SIGUSR1, SIGUSR2);

pcntl_sigprocmask(SIG_BLOCK, $signals);


$now = time();

$today = mktime(0, 0, 0);

$nearest = $today + $wemo->timers['daily'][0]['time'];

if ($now >= $nearest) {

    $nearest += (3600 * 24);
}

$last = $now;

foreach ($wemo->timers['daily'] as $d) {

    $t = $today + $d['time'];

    if ($now >= $t) {

        $t += (3600 * 24);
    }

    if ($t < $nearest) {

        $nearest = $t;
    }
}

pcntl_alarm($nearest - $now);


$running = true;

while ($running) {

    $siginfo = array();

    pcntl_sigwaitinfo($signals, $siginfo);

    if ($siginfo['signo'] == SIGALRM) {

        $today = mktime(0, 0, 0);

        $now = time();

        $nearest = $today + $wemo->timers['daily'][0]['time'];;

        if ($now >= $nearest) {

            $nearest += (3600 * 24);
        }

        foreach ($wemo->timers['daily'] as $d) {

            $t = $today + $d['time'];

            if ($t > $last && $t < $now + 5) {

                switch ($d['action']) {

                    case 'on':

                        $d['plug']->On();

                        break;

                    case 'off':

                        $d['plug']->Off();

                        break;

                    default:

                        break;
                };

                $t += (3600 * 24);
            }

            if ($now >= $t) {

                $t += (3600 * 24);
            }

            if ($t < $nearest) {

                $nearest = $t;
            }
        }

        $last = $now + 5;

        pcntl_alarm($nearest - $now);
    }

    if ($siginfo['signo'] == SIGUSR1) {

        $today = mktime(0, 0, 0);

        $wemo = new WeMo();

        $now = time();

        $nearest = $today + $wemo->timers['daily'][0]['time'];

        if ($now >= $nearest) {

            $nearest += (3600 * 24);
        }

        foreach ($wemo->timers['daily'] as $d) {

            $t = $today + $d['time'];

            if ($now >= $t) {

                $t += (3600 * 24);
            }

            if ($t < $nearest) {

                $nearest = $t;
            }
        }

        $last = $now;

        pcntl_alarm($nearest - $now);
    }


    if ($siginfo['signo'] == SIGUSR2) {

        $today = mktime(0, 0, 0);

        $now = time();

        echo "WeMo\n====\nregistered daily timers:\n";

        $timestamps = array();

        foreach ($wemo->timers['daily'] as $d) {

            $t = $today + $d['time'];

            if ($now >= $t) {

                $t += (3600 * 24);
            }

            $timestamps[] = array(
                'name' => $d['plug']->Name(),
                'action' => $d['action'],
                'time' => $t
            );
        }

        usort($timestamps, function ($a, $b) {

            return $a['time'] - $b['time'];
        });

        foreach ($timestamps as $t) {

            printf("%-16s: %-3s @%s\n", $t['name'], $t['action'], date("D, d M Y H:i:s", $t['time']));
        }
        echo "===\nnext timer at " . date("D, d M Y H:i:s", $nearest) . "\n===\n";
    }

    if ($siginfo['signo'] == SIGINT) {

        $running = false;
    }
}
?>