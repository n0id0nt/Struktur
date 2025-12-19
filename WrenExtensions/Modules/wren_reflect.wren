class Reflect {
  static hasMethod(object, signature) {
    if (object == null) return false
    return hasMethod_(object, signature)
  }
  
  static hasMethodAny(object, methodName) {
    if (object == null) return false
    
    var arities = ["()", "(_)", "(_,_)", "(_,_,_)", "(_,_,_,_)", "(_,_,_,_,_)", "(_,_,_,_,_,_)", "(_,_,_,_,_,_,_)", "(_,_,_,_,_,_,_,_)", "(_,_,_,_,_,_,_,_,_)", "(_,_,_,_,_,_,_,_,_,_)"]
    for (arity in arities) {
      if (hasMethod_(object, methodName + arity)) {
        return true
      }
    }
    return false
  }

  static getMethods(object) {
    if (object == null) return false
    return getMethods_(object)
  }

  static getMethod(class, signature) {
    if (object == null) return false
    return getMethod_(object, signature)
  }

  foreign static hasMethod_(object, signature)
  foreign static getMethods_(object)
  foreign static getMethod_(object, signature)
}
