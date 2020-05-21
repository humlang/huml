#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

struct type_base
{
  virtual void print(std::ostream&) = 0;
};

struct type_table
{
  type_table();

  static constexpr std::size_t Kind_sort_idx = 0;
  static constexpr std::size_t Type_sort_idx = 1;
  static constexpr std::size_t Prop_sort_idx = 2;
  static constexpr std::size_t Unit_idx = 3;

  std::shared_ptr<type_base> operator[](std::size_t idx) { return types[idx]; }

  std::vector<std::shared_ptr<type_base>> types;
};

struct Kind_sort : type_base
{ void print(std::ostream& os) override { os << "Kind"; } };

struct Type_sort : type_base
{ void print(std::ostream& os) override { os << "Type"; } };

struct Prop_sort : type_base
{ void print(std::ostream& os) override { os << "Prop"; } };

// This is for values such as `n` in `Vec n Nat`
struct IdTypeBox : type_base
{
  IdTypeBox(symbol id, std::shared_ptr<type_base> type)
    : id(id), type(type)
  {  }

  void print(std::ostream& os) override { os << id.get_string() << " : "; type->print(os); }

  symbol id;
  std::shared_ptr<type_base> type;
};

// This is interesting. Types and values could have similar references.
struct type_or_value_ref : type_base
{
  type_or_value_ref(symbol name)
    : name(name)
  {  }

  void print(std::ostream& os) override { os << name.get_string(); }

  symbol name;
};

// Sample type_base constructor for equality: eq : Pi (A : Type). Pi (_ : A). Pi (_ : A). Prop
struct type_constructor : IdTypeBox 
{
  type_constructor(symbol id, std::shared_ptr<type_base> type)
    : IdTypeBox(id, type)
  {  }
};

/////////// NON TYPE BEGIN

// These are data constructors such as `Zero : nat` and `Succ : nat -> nat`
struct data_constructor : IdTypeBox
{
  data_constructor(symbol id, std::shared_ptr<type_base> type)
    : IdTypeBox(id, type)
  {  }
};
struct data_constructors
{
  data_constructors(std::shared_ptr<type_base> constructs)
    : constructs(constructs)
  {  }

  std::shared_ptr<type_base> constructs;
  std::vector<data_constructor> data;
};

/////////// NON TYPE END

// Π(x : A). B       where B might use x. Example: Π(n : nat). Vec n
struct pi_type : type_base
{
  pi_type(std::shared_ptr<IdTypeBox> binder, std::shared_ptr<type_base> body)
    : type_base(), binder(binder), body(body)
  {  }

  void print(std::ostream& os) override { os << "\\ ("; binder->print(os); os << "). "; body->print(os); }

  std::shared_ptr<type_base> binder;
  std::shared_ptr<type_base> body; // allowed to use argument
};

// A B
struct application : type_base
{
  application(std::shared_ptr<type_base> A, std::shared_ptr<type_base> B)
    : A(A), B(B)
  {  }

  void print(std::ostream& os) override { A->print(os); os << " "; B->print(os); }

  std::shared_ptr<type_base> A;
  std::shared_ptr<type_base> B;
};


