#!/bin/bash
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

generated_files=()

if [ -z "$1" ]
  then
    echo "regenerate.sh: path to yaml file required"
    exit -1
fi

### Key generation ###
if [ ! -f key_priv.pem ]; then
  echo "Generating private and public keys..."
  rm key_public.c
  suit-generator keys --output-file key
  generated_files+=("\tprivate key:\t\t$PWD/key_priv.pem")
  generated_files+=("\tpublic key:\t\t$PWD/key_pub.pem")
fi
if [ ! -f key_public.c ]; then
  echo "Generating public key as C source file..."
  suit-generator convert --input-file key_priv.pem --output-file key_public.c
  generated_files+=("\tpublic key C file:\t$PWD/key_public.c")
fi

### Manifest generation ###
echo "Generating SUIT envelope for $1 input file ..."
suit-generator create --input-file $1 --output-file sample.suit
generated_files+=("\tunsigned binary envelope:\t\t$PWD/sample.suit")
echo "Signing SUIT envelope using key_priv.pem ..."
suit-generator sign --input-file sample.suit --private-key key_priv.pem --output-file sample_signed.suit
generated_files+=("\tsigned binary envelope:\t\t\t$PWD/sample_signed.suit")
echo "Converting binary envelope into C code ..."
zcbor convert -c ../../../../../modules/lib/suit-processor/cddl/manifest.cddl -c ../../../../../modules/lib/suit-processor/cddl/trust_domains.cddl -i ./sample_signed.suit --input-as cbor -t SUIT_Envelope_Tagged -o sample_signed.suit.c --c-code-var-name manifest --c-code-columns 15
generated_files+=("\tsigned binary envelope as C code:\t$PWD/sample_signed.suit.c")
echo "Done, following files have been created:"
for file in "${generated_files[@]}"
do
    echo -e "$file"
done
