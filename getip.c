#include "include/getip.h"

extern char *getip() {
  // printf("header test\n");

  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];
  static char selhost[NI_MAXHOST];
  
  if(getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }
  else {
    // printf("Success getting interface lists!\n");
  }

  for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if(ifa->ifa_addr == NULL)
      continue;
    family = ifa->ifa_addr->sa_family;
   
    if(family == AF_INET) {
      // printf("%-8s %s (%d)\n", ifa->ifa_name, "fam", family);
      s = getnameinfo(ifa->ifa_addr, 
          (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                sizeof(struct sockaddr_in6),
          host, NI_MAXHOST,
          NULL, 0, NI_NUMERICHOST);
      // printf("addr: %s\n", host);
      if(s != 0) {
        // printf("getnameinfo() failed! %s\n", gai_strerror(s));
      }
      if(strcmp(ifa->ifa_name, "wlan0") == 0) {
        strncpy(selhost, host, NI_MAXHOST);
      }
    }
  }
  // printf("current selhost:%s\n", selhost);
  return selhost;
}
