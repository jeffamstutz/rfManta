
SET (Manta_LoadBalancers_SRCS
     LoadBalancers/CyclicLoadBalancer.h
     LoadBalancers/CyclicLoadBalancer.cc
     LoadBalancers/SimpleLoadBalancer.h
     LoadBalancers/SimpleLoadBalancer.cc
     LoadBalancers/WQLoadBalancer.h
     LoadBalancers/WQLoadBalancer.cc
     )

IF(ENABLE_MPI)
  SET (Manta_LoadBalancers_SRCS ${Manta_LoadBalancers_SRCS}
    LoadBalancers/MPI_LoadBalancer.cc
    LoadBalancers/MPI_LoadBalancer.h
    )
ENDIF(ENABLE_MPI)
