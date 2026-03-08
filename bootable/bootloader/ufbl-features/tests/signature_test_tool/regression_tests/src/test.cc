// Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <string>
#include <climits>
#include "gtest/gtest.h"
#include "ufbl_debug.h"
#include "sig_tests_errors.h"
#include "device_image.h"

enum {
  AMZN_ENGINEERING_DEVICE = 0,
  AMZN_PRODUCTION_DEVICE,
  AMZN_INVALID_DEVICE,
};

enum {
  AMZN_ENGINEERING_CERT = 0,
  AMZN_PRODUCTION_CERT,
};

typedef int (*meta_data_handler)(const char *meta_data);

//This is to store the device type that is passed as a user argument
#ifndef DEVICE_TYPE
#define DEVICE_TYPE
char *device_type;
#endif
//This is to store the signature size (page size)
// that is passed as a user argument
#ifndef SIG_SIZE
#define SIG_SIZE
long int sig_size;
#endif

extern "C" {
  int amzn_image_verify(const void *image,
    unsigned char *signature,
    unsigned int image_size, meta_data_handler handler);
  int amzn_target_device_type(void) {
    return AMZN_ENGINEERING_DEVICE;
  }
  const char *amzn_target_device_name() {
    return device_type;
  }
}

TEST(ImageTest, Validate) {

  device_image image;
  image.load_boot_img();
  int size_of_img_minus_sig = image.get_size() - sig_size;
  EXPECT_EQ(1, amzn_image_verify(image.get_img(), image.get_sig(), size_of_img_minus_sig, 0));
}





TEST(ImageTest, Invalidate) {

  device_image image;
  image.load_boot_img();
  int size_of_img_minus_sig = image.get_size() - sig_size;
  //amzn_image_verify() returns 1 on success
  printf("\nImage size too small\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), size_of_img_minus_sig - 10, 0));
  printf("\nSignature start off by +1\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig() + 1, size_of_img_minus_sig, 0));
  printf("AS OF JULY 12 2017 A ADJUSTING THE POITNER TO THE SIGNATURE BY -1 CAUSES A SEG FAULT\n");
  //printf("\nSignature start off by -1\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig() - 1, size_of_img_minus_sig, 0));
  printf("\nImage pointer off by -1\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img() - 1, image.get_sig(), size_of_img_minus_sig, 0));
}

TEST(ImageTest, NULLS) {

  device_image image;
  image.load_boot_img();
  int size_of_img_minus_sig = image.get_size() - sig_size;
  //amzn_image_verify() returns 1 on success
  printf("AS OF JUNE 21 2017 A NULL IN THE IMAGE OR SIGNATURE POINTERS FIELD WILL CAUSE A SEG FAULT\n");

  //printf("\nImage pointer = 0\n");
  //EXPECT_NE(1, amzn_image_verify(0, image.get_sig(), size_of_img_minus_sig, 0));
  //printf("\nImage pointer and signature pointer = 0\n");
  //EXPECT_NE(1, amzn_image_verify(0, 0, size_of_img_minus_sig, 0));
  //printf("\nImage pointer and image size = 0\n");
  //EXPECT_NE(1, amzn_image_verify(0, image.get_sig(), 0, 0));
  //printf("\nImage pointer, signature pointer, image size = 0\n");
  //EXPECT_NE(1, amzn_image_verify(0, 0, 0, 0));
  //printf("\nSignature pointer = 0\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), 0, size_of_img_minus_sig, 0));
  //printf("\nSignature pointer and image size = 0\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), 0, 0, 0));
  printf("\nImage size = 0\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), 0, 0));
}

TEST(ImageTest, Negatives) {

  device_image image;
  image.load_boot_img();
  int size_of_img_minus_sig = image.get_size() - sig_size;
  //amzn_image_verify() returns 1 on success
  printf("AS OF JUNE 21 2017 A NEGATIVE VALUE IN THE IMAGE OR SIGNATURE POINTER FIELD WILL CAUSE A SEG FAULT (possibly same problem as null)\n");
  //printf("\nImage pointer = -1\n");
  //EXPECT_NE(1, amzn_image_verify((unsigned char*) - 1, image.get_sig(), size_of_img_minus_sig, 0));
  //printf("\nSignature pointer = -1\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), (unsigned char*) - 1, size_of_img_minus_sig, 0));
  printf("AS OF JUNE 21 2017 A NEGATIVE VALUE TO THE META_DATA_HANDLER WILL CAUSE A SEG FAULT (inconsequential)\n");
  //printf("\nMeta_data_handler = -1\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), size_of_img_minus_sig, (int (*)(const char*))-1));
}

TEST(ImageTest, Image_size) {

  device_image image;
  image.load_boot_img();
  int size_of_img_minus_sig = image.get_size() - sig_size;
  //amzn_image_verify() returns 1 on success
  printf("AS OF JUNE 21 2017, GIVING image.get_size() UINT_MAX, UINT_MAX-1, INT_MAX WILL CAUSE A SEG FAULT\n");
  printf("\nImage size = 1\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), 1, 0));
  //printf("\nImage size = unsigned int max\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), UINT_MAX, 0));
  printf("\nImage size = unsigned int max+1\n");
  EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), UINT_MAX + 1, 0));
  //printf("\nImage size = unsigned int max-1\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), UINT_MAX - 1, 0));
  //printf("\nImage size =int max\n");
  //EXPECT_NE(1, amzn_image_verify(image.get_img(), image.get_sig(), INT_MAX, 0));
}

/*
 * Because this test case manipulates global variables like the size of the
 * signature, and what device type is accepted, please keep it as the last
 * test case being run.
 */
TEST(ImageTest, old_fixes) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  device_image image;
  char *corrupted_image;
  int size_of_img_minus_sig;

 /*
  * These crashes only affected libtomcrypt, and will therefore
  * not crash when using Openssl, causing a false failure
  */
#ifdef LIBTOMCRYPT
  //Libtomcrypt: crypt_argchk not testing for NULL
  corrupted_image = "crypt_argchk_null_checking";
  device_type = "ABC";
  sig_size = 2048;
  image.load_corrupted_boot_img(corrupted_image);
  size_of_img_minus_sig = image.get_size()-sig_size;
  printf("Libtomcrypt crypt_argchk NULL checking error\n");
  EXPECT_DEATH(amzn_image_verify(image.get_img(), image.get_sig(), size_of_img_minus_sig, 0), "");

  //Libtomcrypt: off by one error
  corrupted_image = "der_decode_utf8_string_off_by_one";
  device_type = "ABC";
  sig_size = 2048;
  image.load_corrupted_boot_img(corrupted_image);
  size_of_img_minus_sig = image.get_size()-sig_size;
  printf("Libtomcrypt der_decode_utf8_string off-by-one error\n");
  EXPECT_DEATH(amzn_image_verify(image.get_img(), image.get_sig(), size_of_img_minus_sig, 0), "");
#endif
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  sig_size = strtol(argv[2],NULL,10);
  if (sig_size < 0) {
    exit(INPUT_PARAMETERS_OUT_OF_BOUNDS);
  }
  if (argc < 3) {
    fprintf(stderr,"error - not enough input arguments\n");
    fprintf(stderr,"Please pass the device type and page size as input arguments\n");
    exit(NOT_ENOUGH_INPUTS);
  }
  device_type = argv[1];
  return RUN_ALL_TESTS();
}
