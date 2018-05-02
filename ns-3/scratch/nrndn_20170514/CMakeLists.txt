cmake_minimum_required(VERSION 3.8)
project(nrndn_20170514)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        LRUCache.h
        main.cc
        ndn-navigation-route-heuristic-forwarding.cc
        ndn-navigation-route-heuristic-forwarding.h
        ndn-nr-cs-impl.cc
        ndn-nr-cs-impl.h
        ndn-nr-pit-impl.cc
        ndn-nr-pit-impl.h
        ndn-pit-entry-nrimpl.cc
        ndn-pit-entry-nrimpl.h
        NodeSensor.cc
        NodeSensor.h
        NodeSensorHelper.cc
        NodeSensorHelper.h
        nrConsumer.cc
        nrConsumer.h
        nrHeader.cc
        nrHeader.h
        nrndn-Neighbors.cc
        nrndn-Neighbors.h
        nrProducer.cc
        nrProducer.h
        nrUtils.cc
        nrUtils.h
        SumoNodeSensor.cc
        SumoNodeSensor.h
        tradConsumer.cc
        tradConsumer.h)

add_executable(nrndn_20170514 ${SOURCE_FILES})