#include <stdio.h>
#include "vfc_probes.h"

int main(void) {
  volatile double a = 0.1;
  volatile double b = 0.2;
  volatile double c = 0.3;
  double r = (a + b) - c;
  /* Explicit user sync point & flush before MPI/IO */
  vfc_sync_point("mpi_pre_allreduce_residual", r);
  vfc_sync_flush();
  printf("Residual: %g\n", r);
  return 0;
}
