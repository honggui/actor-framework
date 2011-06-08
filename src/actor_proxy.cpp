#include "cppa/atom.hpp"
#include "cppa/message.hpp"
#include "cppa/actor_proxy.hpp"
#include "cppa/exit_reason.hpp"

namespace cppa {

actor_proxy::actor_proxy(std::uint32_t mid, const process_information_ptr& pptr)
    : super(mid), m_parent(pptr)
{
    if (!m_parent) throw std::runtime_error("parent == nullptr");
}

actor_proxy::actor_proxy(std::uint32_t mid, process_information_ptr&& pptr)
    : super(mid), m_parent(std::move(pptr))
{
    if (!m_parent) throw std::runtime_error("parent == nullptr");
}

void actor_proxy::enqueue(const message& msg)
{
    const any_tuple& content = msg.content();
    if (   content.size() > 0
        && content.utype_info_at(0) == typeid(atom_value))
    {
        auto val = *reinterpret_cast<const atom_value*>(content.at(0));
        switch(val)
        {
            case atom(":Link"):
            {
                auto s = msg.sender();
                link_to(s);
                return;
            }
            case atom(":Unlink"):
            {
                auto s = msg.sender();
                unlink_from(s);
                return;
            }
            case atom(":KillProxy"):
            {
                if (   content.size() == 2
                    && content.utype_info_at(1) == typeid(std::uint32_t))
                {
                    const void* reason = content.at(1);
                    cleanup(*reinterpret_cast<const std::uint32_t*>(reason));
                }
                return;
            }
            default: break;
        }
    }
    forward_message(m_parent, msg);
}

void actor_proxy::link_to(intrusive_ptr<actor>& other)
{
    if (link_to_impl(other))
    {
        // causes remote actor to link to (proxy of) other
        forward_message(m_parent, message(this, other, atom(":Link")));
        //enqueue(message(this, other, atom(":Link")));
    }
}

void actor_proxy::unlink_from(intrusive_ptr<actor>& other)
{
    if (unlink_from_impl(other))
    {
        // causes remote actor to unlink from (proxy of) other
        forward_message(m_parent, message(this, other, atom(":Unlink")));
        //enqueue(message(this, other, atom(":Unlink")));
    }
}

bool actor_proxy::establish_backlink(intrusive_ptr<actor>& other)
{
    bool result = super::establish_backlink(other);
    if (result)
    {
        forward_message(m_parent, message(this, other, atom(":Link")));
    }
    //enqueue(message(to, this, atom(":Link")));
    return result;
}

bool actor_proxy::remove_backlink(intrusive_ptr<actor>& other)
{
    bool result = super::remove_backlink(other);
    if (result)
    {
        forward_message(m_parent, message(this, other, atom(":Unlink")));
    }
    //enqueue(message(to, this, atom(":Unlink")));
    return result;
}

const process_information& actor_proxy::parent_process() const
{
    return *m_parent;
}

process_information_ptr actor_proxy::parent_process_ptr() const
{
    return m_parent;
}


} // namespace cppa
