/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once

#include "dnsdist.hh"

class KeyValueLookupKey
{
public:
  virtual ~KeyValueLookupKey()
  {
  }
  virtual std::vector<std::string> getKeys(const DNSQuestion&) = 0;
  virtual std::string toString() const = 0;
};

class KeyValueLookupKeySourceIP: public KeyValueLookupKey
{
public:
  std::vector<std::string> getKeys(const DNSQuestion& dq) override
  {
    std::vector<std::string> result;

    if (dq.remote->sin4.sin_family == AF_INET) {
      result.emplace_back(reinterpret_cast<const char*>(&dq.remote->sin4.sin_addr.s_addr), sizeof(dq.remote->sin4.sin_addr.s_addr));
    }
    else if (dq.remote->sin4.sin_family == AF_INET6) {
      result.emplace_back(reinterpret_cast<const char*>(&dq.remote->sin6.sin6_addr.s6_addr), sizeof(dq.remote->sin6.sin6_addr.s6_addr));
    }

    return result;
  }

  std::string toString() const override
  {
    return "source IP";
  }
};

class KeyValueLookupKeyQName: public KeyValueLookupKey
{
public:
  std::vector<std::string> getKeys(const DNSQuestion& dq) override
  {
    return {dq.qname->toDNSStringLC()};
  }

  std::string toString() const override
  {
    return "qname";
  }
};

class KeyValueLookupKeySuffix: public KeyValueLookupKey
{
public:
  std::vector<std::string> getKeys(const DNSQuestion& dq) override
  {
    auto lowerQName = dq.qname->makeLowerCase();
    std::vector<std::string> result(lowerQName.countLabels());

    do {
      result.emplace_back(lowerQName.toDNSString());
    }
    while (lowerQName.chopOff());

    return result;
  }

  std::string toString() const override
  {
    return "qname";
  }
};

class KeyValueLookupKeyTag: public KeyValueLookupKey
{
public:
  KeyValueLookupKeyTag(const std::string& tag): d_tag(tag)
  {
  }

  std::vector<std::string> getKeys(const DNSQuestion& dq) override
  {
    if (dq.qTag) {
      const auto& it = dq.qTag->find(d_tag);
      if (it != dq.qTag->end()) {
        return { it->second };
      }
    }
    return {};
  }

  std::string toString() const override
  {
    return " value of the tag named '" + d_tag + '"';
  }

private:
  std::string d_tag;
};

class KeyValueStore
{
public:
  virtual ~KeyValueStore()
  {
  }

  virtual bool getValue(const std::string& key, std::string& value) = 0;
};

#ifdef HAVE_LMDB

#include "lmdb-safe.hh"

class LMDBKVStore: public KeyValueStore
{
public:
  LMDBKVStore(const std::string& fname, const std::string& dbName): d_env(fname.c_str(), MDB_NOSUBDIR, 0600), d_fname(fname), d_dbName(dbName)
  {
  }

  bool getValue(const std::string& key, std::string& value) override;

private:
  MDBEnv d_env;
  std::string d_fname;
  std::string d_dbName;
};

#endif /* HAVE_LMDB */

#ifdef HAVE_CDB

#include "cdb.hh"

class CDBKVStore: public KeyValueStore
{
public:
  CDBKVStore(const std::string& fname): d_cdb(fname), d_fname(fname)
  {
  }

  bool getValue(const std::string& key, std::string& value) override;

private:
  CDB d_cdb;
  std::string d_fname;
};

#endif /* HAVE_LMDB */
