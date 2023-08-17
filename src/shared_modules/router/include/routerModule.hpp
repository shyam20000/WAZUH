/*
 * Wazuh router
 * Copyright (C) 2015, Wazuh Inc.
 * March 25, 2023.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _ROUTER_MODULE_HPP
#define _ROUTER_MODULE_HPP

#if __GNUC__ >= 4
#define EXPORTED __attribute__((visibility("default")))
#else
#define EXPORTED
#endif

#include "logging_helper.h"
#include "singleton.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief RouterModule
 *
 */
class EXPORTED RouterModule final : public Singleton<RouterModule>
{
public:
    /**
     * @brief
     *
     * @param logFunction
     */
    static void initialize(const std::function<void(const modules_log_level_t, const std::string&)>& logFunction);

    /**
     * @brief start
     *
     */
    void start();

    /**
     * @brief stop
     *
     */
    void stop();
};

#endif //_ROUTER_MODULE_HPP
