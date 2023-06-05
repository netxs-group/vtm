// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

#include "application.hpp"

namespace netxs::scripting
{
    using namespace ui;

    namespace path
    {
        static constexpr auto scripting = "/config/scripting/";
    }
    namespace attr
    {
        static constexpr auto cwd = "cwd";
        static constexpr auto cmd = "cmd";
        static constexpr auto run = "run";
        static constexpr auto tty = "usetty";
        static constexpr auto rse = "engine"; // Runtime Scripting Engine.
    }

    struct nullterm
    {
        bool  altbuf{}; // nullterm: Altbuf buffer stub.
        bool  normal{}; // nullterm: Normal buffer stub.
        bool  io_log{}; // nullterm: Stdio logging flag.
        bool* target{&normal}; // nullterm: Current buffer stub.
    };

    template<class Host>
    struct repl
        : public nullterm
    {
        using s11n = directvt::binary::s11n;
        using pidt = os::pidt;
        using vtty = sptr<os::runspace::basetty>;

        Host& owner;
        // repl: Event handler.
        class xlat
            : public s11n
        {
            Host& owner; // xlat: Boss object reference.
            subs  token; // xlat: Subscription tokens.

        public:
            void disable()
            {
                token.clear();
            }

            void handle(s11n::xs::fullscreen          lock)
            {
                //...
            }
            void handle(s11n::xs::focus_cut           lock)
            {
                //...
            }
            void handle(s11n::xs::focus_set           lock)
            {
                //...
            }
            void handle(s11n::xs::keybd_event         lock)
            {
                //...
            };

            xlat(Host& owner)
                : s11n{ *this },
                 owner{ owner }
            {
                owner.LISTEN(tier::anycast, e2::form::prop::ui::header, utf8, token)
                {
                    //s11n::form_header.send(owner, 0, utf8);
                };
                owner.LISTEN(tier::anycast, e2::form::prop::ui::footer, utf8, token)
                {
                    //s11n::form_footer.send(owner, 0, utf8);
                };
                owner.LISTEN(tier::release, hids::events::device::mouse::any, gear, token)
                {
                    //...
                };
                owner.LISTEN(tier::general, hids::events::die, gear, token)
                {
                    //...
                };
            }
        };

        xlat stream; // repl: Event tracker.
        text curdir; // repl: Current working directory.
        text cmdarg; // repl: Startup command line arguments.
        flag active; // repl: Scripting engine lifetime.
        pidt procid; // repl: PTY child process id.
        vtty engine; // repl: Scripting engine instance.

        // repl: Proceed input.
        void ondata(view data)
        {
            //if (active)
            {
                log(ansi::fgc(greenlt).add(data).nil(), faux);
                //stream.s11n::sync(data);
            }
        }
        // repl: Cooked read input.
        void data(rich& data)
        {
            owner.bell::trysync(active, [&]
            {
                log(ansi::fgc(cyanlt).add(data.utf8()).nil(), faux);
            });
        }
        // repl: Shutdown callback handler.
        void onexit(si32 code, view msg = {})
        {
            //todo initiate global shutdown

            //netxs::events::enqueue(owner.This(), [&, code](auto& boss) mutable
            //{
            //    if (code) log(ansi::bgc(reddk).fgc(whitelt).add('\n', prompt::repl, "Exit code ", utf::to_hex_0x(code), ' ').nil());
            //    else      log(prompt::repl, "Exit code 0");
            //    //backup.reset(); // Call repl::dtor.
            //});
        }
        // repl: .
        template<class P>
        void update(P api_proc)
        {
            owner.bell::trysync(active, [&]
            {
                api_proc();
            });
        }
        // repl: Write client data.
        void write(view data)
        {
            if (!engine) return;
            log(prompt::repl, "exec: ", ansi::hi(utf::debase<faux, faux>(data)));
            engine->write(data);
        }
        // repl: Start a new process.
        void start(text cwd, text cmd)
        {
            if (!engine) return;
            curdir = cwd;
            cmdarg = cmd;
            if (!engine->connected())
            {
                procid = engine->start(curdir, cmdarg, os::ttysize, [&](auto utf8_shadow) { ondata(utf8_shadow); },
                                                                    [&](auto code, auto msg) { onexit(code, msg); });
            }
        }
        void shut()
        {
            active = faux;
            if (!engine) return;
            if (engine->connected()) engine->shut();
        }

        repl(Host& owner)
            : owner{ owner },
              stream{owner },
              active{ true }
        {
            auto& config = owner.config;
            if (config.take(path::scripting, faux))
            {
                config.pushd(path::scripting);
                auto rse = config.take(attr::rse, ""s);
                config.cd(rse);
                auto cwd = config.take(attr::cwd, ""s);
                auto cmd = config.take(attr::cmd, ""s);
                auto run = config.take(attr::run, ""s);
                auto tty = config.take(attr::tty, faux);
                if (tty) engine = ptr::shared<os::runspace::tty<repl>>(*this);
                else     engine = ptr::shared<os::runspace::raw>();
                start(cwd, cmd);
                //todo run integration script
                if (run.size()) write(run + "\n");
                config.popd();
            }
        }
    };
}