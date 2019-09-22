#include <sstream>
#include <stack>
#include <memory>
#include <utility>

#include "reader.hpp"

class compiler
{
public:
    compiler(memory& memory);

    void compile(const std::vector<expression::ptr>& ast);

private:
    class memory& memory;
};

compiler::compiler(class memory& memory):
    memory(memory)
{
}

void compiler::compile(const std::vector<expression::ptr>& ast)
{
    std::stack<expression::ptr> stack;
    for(auto expr:ast)
    {
        stack.push(expr);
    }

    while(!stack.empty())
    {
        auto expression = stack.top();
        stack.pop();

        auto l = expression->as<list>();
        if(l)
        {
            auto operation = l->children()[0]->as<atom>();
            for(auto it = l->children().rbegin(); it != l->children().rend()-1; ++it)
            {
                auto atom = (*it)->as<::atom>();
                auto i = std::stoi(atom->to_symbol().get_string());
                memory << instruction_type::PUSH << integer(i);
            }

            for(int i = 1; i < l->children().size(); ++i)
            {
                memory << instruction_type::ADD;
            }
        }
    }
}

class traverse_event
{
};

class traverser
{
public:
    using ptr = std::unique_ptr<traverser>;

    virtual ~traverser() = default;

    void start(std::vector<expression::ptr>& tree);

    virtual void traverse(traverse_event& event) = 0;

private:
};

void traverser::start(std::vector<expression::ptr>& tree)
{
    traverse_event event;
    traverse(event);
}

class traverser_container : public traverser
{
public:
    using children_type = std::vector<traverser::ptr>;

    template <typename... Args>
    void add_child(Args&&... args);
    void add_child(traverser::ptr&& ptr);

    virtual void traverse(traverse_event& event);

private:
    children_type children;
};

template <typename... Args>
void traverser_container::add_child(Args&&... args)
{
  // FIXME: this is super buggy. you call forward with multiple arguments... and "add_children" doesn't exist
    add_children(std::make_unique(std::forward(args...)));
}

void traverser_container::add_child(traverser::ptr&& ptr)
{
    children.push_back(std::move(ptr));
}

void traverser_container::traverse(traverse_event& event)
{
    for(auto child = children.begin(); child != children.end(); ++child)
    {
        (*child)->traverse(event);
    }
}

int main()
{
    std::stringstream ss("(+ 1 2 3 4)");
    auto ast = reader::read("STATIC", ss);

    memory instruction_memory;
    memory stack_memory;
    stack stack(stack_memory);

    compiler compiler(instruction_memory);
    compiler.compile(ast);

    instruction_memory << instruction_type::DUMP
                       << instruction_type::HALT;

    executor e(stack, instruction_memory);

    e.main();

    return 0;
}
