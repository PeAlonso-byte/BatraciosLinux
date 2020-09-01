#!/bin/bash

echo "Uso: ./eliminar.sh <id> <-m|-s>"
echo ejemplo de uso, quiero borrar mis semaforos:
echo ./eliminar.sh i0000000 s
echo borrar memoria:
echo ./eliminar.sh i0000000 m
ipcs | grep $1 | grep -w $2 | cut -c 3-12 | grep -v '^$' | xargs -L1 ipcrm -$2
echo Acabado.
