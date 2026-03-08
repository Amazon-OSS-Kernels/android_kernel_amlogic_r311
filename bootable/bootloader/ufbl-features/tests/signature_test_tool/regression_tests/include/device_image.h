/*
 * Copyright 2019 Amazon.com, Inc. or its Affiliates. All rights reserved.
 */
#ifndef REGRESSION_TESTS_INCLUDE_DEVICE_IMAGE_H
#define REGRESSION_TESTS_INCLUDE_DEVICE_IMAGE_H

//This is to store the signature size (page size)
// that is passed as a user argument
#ifndef SIG_SIZE
#define SIG_SIZE
long int sig_size;
#endif
#ifndef DEVICE_TYPE
#define DEVICE_TYPE
//This is to store the device type that is passed as a user argument
char *device_type;
#endif
class device_image {
  public:
    device_image();
    ~device_image();
    void load_boot_img();
    void load_corrupted_boot_img(char *corrupted_image);
    unsigned char* get_img() {return img;}
    unsigned char* get_sig() {return sig;}
    int get_size() {return size;}
  private:
    unsigned char *sig;
    unsigned char *img;
    int size;
};

device_image::device_image() {
  sig = NULL;
  img = NULL;
  size = 0;
}

device_image::~device_image() {
  if (img != NULL)
    delete[] img;
}

void device_image::load_boot_img() {
  char path[PATH_MAX];
  strncpy(path, "../include/", 13);
  strcat(path, device_type);
  strcat(path, "/boot.img");
  FILE *amzn_boot_img = fopen( path, "r" );
  if(amzn_boot_img == NULL) {
    fprintf(stderr, "error - file was not opened.\n"
      "The device type may have been "
      "misspelled, or there may not be a "
      "boot.img file in the "
      "signature_test_tool/include/<device_type> "
      "directory\n");
    exit(CANNOT_OPEN_FILE);
  }
  //Determine size of file (limit 2GB)
  fseek(amzn_boot_img, 0L, SEEK_END);
  size = ftell(amzn_boot_img);
  rewind(amzn_boot_img);
  img = (unsigned char*)malloc(size);
  if (img == NULL) {
    fprintf(stderr, "error - unable to allocate memory");
    exit(UNABLE_TO_ALLOCATE_MEMORY);
  }
  fread(img, 1, size, amzn_boot_img);
  sig = img;
  //Make sig point to beginning of signature
  sig += size-sig_size;
  fclose(amzn_boot_img);
  return;
}
void device_image::load_corrupted_boot_img(char *corrupted_image) {
  char path[PATH_MAX];
  strncpy(path, "include/", 9);;
  strcat(path, "corrupted_images/");
  strcat(path, corrupted_image);
  strcat(path, "/boot.img");
  FILE *amzn_boot_img = fopen( path, "r" );
  if(amzn_boot_img == NULL) {
    fprintf(stderr, "error - file was not opened.\n"
      "The device type may have been "
      "misspelled, or there may not be a "
      "boot.img file in the "
      "signature_test_tool/include/<device_type> "
      "directory\n");
    exit(CANNOT_OPEN_FILE);
  }
  //Determine size of file (limit 2GB)
  fseek(amzn_boot_img, 0L, SEEK_END);
  size = ftell(amzn_boot_img);
  rewind(amzn_boot_img);
  img = (unsigned char*)malloc(size);
  if (img == NULL) {
    fprintf(stderr, "error - unable to allocate memory");
    exit(UNABLE_TO_ALLOCATE_MEMORY);
  }
  fread(img, 1, size, amzn_boot_img);
  sig = img;
  //Make sig point to beginning of signature
  sig += size-sig_size;
  fclose(amzn_boot_img);
  return;
}
#endif //REGRESSION_TESTS_INCLUDE_GET_IMAGE_H
