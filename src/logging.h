#pragma once

#include "log4cpp/Category.hh"
#include "log4cpp/Configurator.hh"
#include "log4cpp/Priority.hh"

log4cpp::Category& logging_init(log4cpp::Priority::PriorityLevel);
