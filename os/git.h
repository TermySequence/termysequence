// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if USE_LIBGIT2
#include <git2.h>

#define DECLSYM(x) typeof(x) *p_ ## x
extern DECLSYM(git_branch_name);
extern DECLSYM(git_branch_upstream);
extern DECLSYM(git_buf_free);
extern DECLSYM(git_describe_commit);
extern DECLSYM(git_describe_format);
extern DECLSYM(git_describe_result_free);
extern DECLSYM(git_graph_ahead_behind);
extern DECLSYM(git_object_free);
extern DECLSYM(git_reference_free);
extern DECLSYM(git_reference_name);
extern DECLSYM(git_reference_name_to_id);
extern DECLSYM(git_reference_peel);
extern DECLSYM(git_reference_shorthand);
extern DECLSYM(git_repository_free);
extern DECLSYM(git_repository_head);
extern DECLSYM(git_repository_head_detached);
extern DECLSYM(git_repository_open_ext);
extern DECLSYM(git_repository_workdir);
extern DECLSYM(git_status_file);

#ifndef LIBGIT2_NO_IFACE
#define git_branch_name(...) (*p_git_branch_name)(__VA_ARGS__)
#define git_branch_upstream(...) (*p_git_branch_upstream)(__VA_ARGS__)
#define git_buf_free(...) (*p_git_buf_free)(__VA_ARGS__)
#define git_describe_commit(...) (*p_git_describe_commit)(__VA_ARGS__)
#define git_describe_format(...) (*p_git_describe_format)(__VA_ARGS__)
#define git_describe_result_free(...) (*p_git_describe_result_free)(__VA_ARGS__)
#define git_graph_ahead_behind(...) (*p_git_graph_ahead_behind)(__VA_ARGS__)
#define git_object_free(...) (*p_git_object_free)(__VA_ARGS__)
#define git_reference_free(...) (*p_git_reference_free)(__VA_ARGS__)
#define git_reference_name(...) (*p_git_reference_name)(__VA_ARGS__)
#define git_reference_name_to_id(...) (*p_git_reference_name_to_id)(__VA_ARGS__)
#define git_reference_peel(...) (*p_git_reference_peel)(__VA_ARGS__)
#define git_reference_shorthand(...) (*p_git_reference_shorthand)(__VA_ARGS__)
#define git_repository_free(...) (*p_git_repository_free)(__VA_ARGS__)
#define git_repository_head(...) (*p_git_repository_head)(__VA_ARGS__)
#define git_repository_head_detached(...) (*p_git_repository_head_detached)(__VA_ARGS__)
#define git_repository_open_ext(...) (*p_git_repository_open_ext)(__VA_ARGS__)
#define git_repository_workdir(...) (*p_git_repository_workdir)(__VA_ARGS__)
#define git_status_file(...) (*p_git_status_file)(__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int osLoadLibgit2();

#ifdef __cplusplus
}
#endif

#endif
