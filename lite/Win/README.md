# Screen Share Windows Desktop

## Overview

This is my attempt of getting a working windows solution for my LAN screen sharing app. It uses Win32 API for the client side application with Winsock for TCP communication.

## Building

Open 'Client.slnx' and 'Server.slnx in visual studio and build as usual from there.
Alternatively use the pre-built executables in the current directory.

## Use

Run the server application on the device that is sharing the screen. Then run the client application, input the server IP address and the client should connect. This should not be used outside of a controled environment.
