//
// Created by Nico on 11/14/17.
//

#ifndef JODA_IPROJECTOR_H
#define JODA_IPROJECTOR_H
#include <joda/document/RapidJsonDocument.h>
#include <rapidjson/pointer.h>

namespace joda::query {
/**
 * Interface representing a single transformation used by the AS statement
 * This interface only represents 1:1 transformations
 */
class IProjector {
 public:
  /**
   * Initializes one IProjector with a given destination pointer-string
   * @param to the pointer-string where the transformation result should be
   * stored
   */
  IProjector(const std::string &to) : ptr(to.c_str()), ptr_str(to){};

  virtual ~IProjector() = default;

  /**
   * Projects a value from a source document into the destination document.
   * This function uses the (overwritten) getVal function to retrieve the value
   * and stores the result at the to-pointer in the new document
   * @param json the source document, used to retrieve values from
   * @param newDoc the destination document, used to store transformed values in
   */
  void project(const RapidJsonDocument &json, RJDocument &newDoc) {
    auto val = this->getVal(json, newDoc.GetAllocator());
    if (!val.IsNull()) ptr.Set(newDoc, std::move(val));
  }

  /**
   * Returns the destination pointer string
   * @return the destination pointer string
   */
  std::string getToPointer() const { return ptr_str; }

  /**
   * Returns a string representing the transformation function
   * @return
   */
  virtual std::string getType() = 0;

  /**
   * Returns a string representing the whole transformation as it would be
   * specified in a query
   * @return
   */
  virtual std::string toString() { return "'" + ptr_str + "':"; }

 protected:
  virtual RJValue getVal(const RapidJsonDocument &json,
                         RJMemoryPoolAlloc &alloc) = 0;

  RJPointer ptr;
  std::string ptr_str;
};
}
#endif  // JODA_IPROJECTOR_H
