# Project：Wi-Fi bridge Project

## Introduction
bridge, is the middle of the continuation, the extension of the extension of the meaning. 
The main role of wireless bridge is to extend the WiFi signal, after the extension of WiFi, 
so that the original coverage of WiFi places can not have WiFi, free of wiring troubles. 
The two wireless routers are bridged together to achieve full coverage of WiFi signals.

* Run the demo as station mode and join the target AP
```sh
bridge open <softap_ssid> <Router's ssid> [Router's passphrase]
```


## Special Macro Configuration Description：

```
CONFIG_BRIDGE=y       // support lwip bridge

```

## Complie Command:

```sh
make bk7236 PROJECT=wifi/bridge
```

(Note: all-app.bin located in `armino/build/bridge/bk7236` )




