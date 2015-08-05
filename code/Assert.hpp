#ifndef ASSERT_HPP
#define ASSERT_HPP

#include <string>

#include <QtWidgets/QMessageBox>

#include "Printf.hpp"
#include "ToString.hpp"

#define MOJO_ASSERT_HELPER(expression, modalDialogTitle)                                                                                                               \
    do                                                                                                                                                                 \
    {                                                                                                                                                                  \
        if (!(expression))                                                                                                                                             \
        {                                                                                                                                                              \
            std::string assertMessage       = mojo::toString("Filename: ", __FILE__, "\n\n\n\nLine Number: ", __LINE__, "\n\n\n\nExpression: ", #expression );         \
            std::string assertMessagePrintf = mojo::toString("Filename: ", __FILE__, "\n\nLine Number: ",     __LINE__, "\n\nExpression: ",     #expression );         \
            mojo::printf("\n\n\n", assertMessagePrintf, "\n\n\n");                                                                                                     \
            QMessageBox messageBox;                                                                                                                                    \
            messageBox.setText(modalDialogTitle);                                                                                                                      \
            messageBox.setInformativeText(QString(assertMessage.c_str()));                                                                                             \
            messageBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Ignore | QMessageBox::Retry);                                                              \
            int response = messageBox.exec();                                                                                                                          \
            bool debugBreak = false;                                                                                                                                   \
            switch(response)                                                                                                                                           \
            {                                                                                                                                                          \
                case QMessageBox::Abort:  exit( -1 );                                                                                                                  \
                case QMessageBox::Ignore: break;                                                                                                                       \
                case QMessageBox::Retry:  debugBreak = true; break;                                                                                                    \
                default:                  debugBreak = true; break;                                                                                                    \
            }                                                                                                                                                          \
            if (debugBreak)                                                                                                                                            \
            {                                                                                                                                                          \
                __asm__("int $3");                                                                                                                                     \
                debugBreak = false;                                                                                                                                    \
            }                                                                                                                                                          \
            while (debugBreak)                                                                                                                                         \
            {                                                                                                                                                          \
            }                                                                                                                                                          \
        }                                                                                                                                                              \
    } while (0)                                                                                                                                                        \

#define MOJO_RELEASE_ASSERT(expression) MOJO_ASSERT_HELPER(expression, "MOJO_RELEASE_ASSERT")

#ifdef MOJO_DEBUG
#define MOJO_ASSERT(expression) MOJO_ASSERT_HELPER(expression, "MOJO_ASSERT")
#else
#define MOJO_ASSERT(expression)
#endif

#endif
