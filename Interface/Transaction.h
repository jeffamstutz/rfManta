
#ifndef Manta_Interface_Transaction_h
#define Manta_Interface_Transaction_h

#include <Interface/TValue.h>
#include <Core/Util/Callback.h>
#include <iostream>
#include <typeinfo>

namespace Manta {
  class MantaInterface;
  class Object;

  class TransactionBase {
  public:
    // Queue loop behavior following this transaction.
    enum {
      DEFAULT  = 0, // Process this transaction and then the next.
                    // (Until either all are processed or another flag is encountered.)
      CONTINUE,     // Process this transaction and then continue render.
                    // (Other transactions processed following the next frame.)
      PURGE,        // Process this transaction and then purge all remaining from the queue.
      NO_UPDATE     // This transaction shouldn't update the changed bit for the pipeline
    };

    virtual void apply() = 0;
    virtual void printValue(std::ostream&) = 0;
    virtual void printOp(std::ostream&) = 0;
    virtual ~TransactionBase();

    const char* getName() const { return name; }
    int getFlag() const { return flag; };

  protected:
    TransactionBase(const char* name, int flag_=TransactionBase::DEFAULT );
  private:
    TransactionBase(const TransactionBase&);
    TransactionBase& operator=(const TransactionBase&);

    const char* name;
    int flag;
  };

  template<class T, class Op> class Transaction : public TransactionBase {
  public:
    Transaction(const char* name, TValue<T>& value, Op op, int flag = TransactionBase::DEFAULT )
      : TransactionBase(name), value(value), op(op)
    {   }

    virtual void apply()
    {
      value = op(value);
    }

    virtual void printValue(std::ostream& stream)
    {
      stream << value.value;
    }

    virtual void printOp(std::ostream& stream)
    {
      stream << typeid(op).name();
    }

    virtual ~Transaction()
    {
    }

  private:
    Transaction(const Transaction<T, Op>&);
    Transaction<T, Op>& operator=(const Transaction<T, Op>&);

    TValue<T>& value;
    Op op;
  };

  class CallbackTransaction : public TransactionBase {
  public:
    CallbackTransaction(const char* name, CallbackBase_0Data* callback, int flag)
      : TransactionBase(name,flag), callback(callback)
    { }

    virtual void apply()
    {
      callback->call();
      delete callback;
    }

    virtual void printValue(std::ostream& stream)
    {
      stream << "(n/a)";
    }

    virtual void printOp(std::ostream& stream)
    {
      stream << typeid(*callback).name();
    }
  private:
    CallbackTransaction(const CallbackTransaction&);
    CallbackTransaction& operator=(const CallbackTransaction&);

    CallbackBase_0Data* callback;
  };

  class UpdateTransaction : public TransactionBase {
  public:
    UpdateTransaction(const char* name, CallbackBase_0Data* callback, Object* object, int flag)
      : TransactionBase(name,flag), callback(callback), object(object)
    { }

    virtual void apply()
    {
      callback->call();
      delete callback;
    }

    virtual void printValue(std::ostream& stream)
    {
      stream << "(n/a)";
    }

    virtual void printOp(std::ostream& stream)
    {
      stream << typeid(*callback).name();
    }

    virtual Object* getObject() const {
      return object;
    }
  private:
    UpdateTransaction(const UpdateTransaction&);
    UpdateTransaction& operator=(const UpdateTransaction&);

    CallbackBase_0Data* callback;
    Object* object;
  };
}

#endif
