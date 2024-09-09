# TCP Rate Limiter Kernel Module Project

This project consists of a kernel module that implements a TCP connection rate limiter and a Python server script for testing.

## TCP Rate Limiter Kernel Module

This module implements a TCP connection rate limiter.

To compile and load the module:

```
make
sudo insmod tcp_rate_limiter.ko
```

## Python Server

This is a simple server written in Python for testing the rate limiter.

To run the server:

```
python3 server.py
```

The server will start listening on 127.0.0.1:8000.

## Testing with Apache Benchmark (ab)

To test the rate limiter, you can use Apache Benchmark (ab). First, make sure you have ab installed. On Ubuntu, you can install it with:

```
sudo apt-get install apache2-utils
```

Then, in a separate terminal, run:

```
ab -n 100 -c 10 http://127.0.0.1:8000/
```

This command sends 100 requests with a concurrency of 10 to the server. You should see some requests being dropped due to the rate limit.

## Monitoring Kernel Messages

There are two ways to view the kernel messages and see the TCP rate limiter in action:

1. For real-time monitoring, use:

```
sudo dmesg -wH
```

This will display kernel messages in real-time.

2. To view the most recent messages, use:

```
dmesg | tail
```

This shows the last few kernel messages, which is useful for quickly checking the latest activity.

You should see output from the rate limiter when connections are dropped, similar to:

```
[  +0.000000] Rate limit exceeded for IP 127.0.0.1
```

## Removing the Kernel Module

To unload the module:

```
sudo rmmod tcp_rate_limiter
```

## Notes

- Make sure you have the necessary kernel headers installed to compile the kernel modules.
- The TCP rate limiter is set to allow a maximum of 5 connections per IP address within a 5-second window.