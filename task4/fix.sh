#!/bin/bash

touch currtime
find . -newer currtime -exec touch {} \;
rm currtime