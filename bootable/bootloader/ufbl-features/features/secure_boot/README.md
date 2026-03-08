# Kernel Image Signature Troubleshooting

## Get the page size of boot.img:

in hex format:

```od -j 36 -t x4 -N 4 -An <boot.img image_path>```

Convert from hexadecimal to  decimal format:

```echo "ibase=16; <hex_value>" | bc```

above steps give ```page_size``` in decimal format

Save the Signature (ASN.1 encoded) which is 1 page long and at the end of the boot.img

```tail -c $page_size > boot_sig```

file boot_sig obtained is file containing signature in ASN.1 format

## To print the certificate

```cat boot_sig | openssl x509 -inform der```

## To print the ASN.1 decoded data:

```openssl asn1parse -inform der -in boot_sig```

Kernel image (boot.img) signature ASN.1 contains the data structures and provides information about the signature.

