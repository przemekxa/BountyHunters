#!/bin/bash
mpirun --oversubscribe  \
    -np 3               \
    main 	            \
    shopSize=1			\
    minOrders=2			\
    maxOrders=5			\
    hunterMin=1			\
    hunterMax=2			\
    storeWaitMin=1		\
    storeWaitMax=3		\
    missionWaitMin=1	\
    missionWaitMax=3     
