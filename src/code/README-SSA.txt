SSA code generation for Pike
============================

The experimental machine-state SSA model is in ssa.[ch]. Enable it with
--enable-ssa. The model assigns versions to the interpreter pointer,
frame pointer, stack pointer, mark stack pointer, and derived locals base
as code is emitted. This is an initial machine-state IR, not a full
machine-instruction SSA IR. PIKE_SSA_STATS=1 prints aggregate counts of
state loads, definitions, stores, and generated code bytes at process exit.

Source organization
-------------------

ssa.h declares the generic machine-state API; ssa.c implements it.
../pikecode.h includes the declarations and ../pikecode.c includes the
implementation when PIKE_ENABLE_SSA is defined. These are internal compiler
interfaces. Do not compile ssa.c as a separate object as well.

The generic model describes cached interpreter state during code emission.
It keeps the latest operation for each slot and coalesces pending stores;
it does not retain a complete instruction graph or emit machine code.
The backend owns physical registers, native instructions, deferred address
biases, cache-valid flags, source PCs, relocations and branch patching.

ssa_integer.[ch] is a separate, bounded integer-value planner. Its value
indices, logical banks and snapshots describe integer operands, not the
interpreter-state slots in ssa.h. Backend-local optimizers may maintain
additional graphs: amd64_ssa.c, for example, buffers native regions and
tracks their layout and local-base uses. Those graphs are not supplied by
the generic machine-state API.

CPU-specific register assignments, enabled optimizations and comparison
flags are documented in the opening comments of the corresponding CPU
files. Start with amd64.c and sparc.c for existing integrations.

Generic state model
-------------------

Keep a struct pikecode_ssa_state in the backend's code-generator state.
Initialize it with pikecode_ssa_reset() before tracking operations. The
slots in enum pikecode_ssa_slot are:

  PIKE_SSA_IP        Interpreter pointer.
  PIKE_SSA_SP        Pike value-stack pointer.
  PIKE_SSA_FP        Pike frame pointer.
  PIKE_SSA_MARK_SP   Pike mark-stack pointer.
  PIKE_SSA_LOCALS    Read-only locals base derived from the current frame.

SP and MARK_SP have write-back targets in the interpreter structure; FP
comes from its frame_pointer field. IP's canonical source depends on the
backend's entry convention. LOCALS is a derived cache, not an independent
interpreter field to write back. Invalidate it when its frame or physical
register becomes invalid, or external effects can change its base.

For each slot, memory[] identifies its canonical interpreter-state version
(including external or derived inputs), and reg[] identifies the version
represented by the backend's cache. A cached value can include a register
plus a deferred address bias.
Value numbers are compile-time identities, not runtime pointer values or
physical register numbers. Zero in reg[] means unknown/unavailable; the
nonzero numbers belong to one state-reset interval.

A nonzero reg[] equal to memory[] describes a clean cached value. A
pending definition can make reg[] newer than memory[]: the backend's value
is current, but interpreter memory still needs a store. Native cache-valid
flags must agree with this bookkeeping; the model cannot check the actual
contents of a machine register.

value[] records the latest operation for each slot: PIKE_SSA_LOAD,
PIKE_SSA_DEFINE, PIKE_SSA_PHI or PIKE_SSA_CLOBBER. pending_store[] holds at
most one PIKE_SSA_STORE per slot. Each operation records its slot, value
number and block number. A PIKE_SSA_NONE record has no valid payload; do
not inspect its other fields. next_value and block are bookkeeping
counters, not an API for constructing a control-flow graph.

API reference
-------------

All functions below take a struct pikecode_ssa_state * as their first
argument. slot is an enum pikecode_ssa_slot and kind is an enum
pikecode_ssa_barrier_kind. All return void except take_store(), which
returns int and takes a struct pikecode_ssa_op * as its last argument.
They update the model only; the backend emits corresponding loads,
arithmetic and stores itself.

  pikecode_ssa_reset(state)
    Start a fresh numbering interval, clear pending stores and set block
    to zero. Give every memory slot an initial version. All register
    versions start unknown except IP, which is assumed to be supplied
    in a persistent register. A backend with a lazily loaded IP must
    clobber that slot after reset, as sparc_flush_codegen_state() does.
    Materialize existing pending stores before resetting; reset itself
    does not flush them or emit a function prologue.

  pikecode_ssa_read(state, slot)
    Record that the cache contains the current canonical memory version,
    usually after a native load. This can also record a value established
    by the entry ABI. Memory must be current: a pending store for this
    slot is an error. Reuse an already valid cached value instead of
    loading stale memory over a definition.

  pikecode_ssa_define(state, slot)
    Assign a fresh version to an updated cached value and schedule its
    store. Emit or record the native update separately. Repeated defines
    replace the pending store with the latest version, allowing one final
    write-back. They do not remove previously emitted arithmetic.

  pikecode_ssa_take_store(state, slot, op)
    Return 1 and copy a pending STORE to *op, or return 0 without writing
    *op if none exists. Taking the store clears it and advances memory[]
    to the stored version; reg[] remains valid and clean. Emit the native
    store as part of that write-back. Normalize any deferred bias first.
    The model assumes the store will execute on the current path; do not
    take a store and then discard its emission.

  pikecode_ssa_clobber(state, slot)
    Give memory a fresh unknown-to-the-cache version, clear reg[] and
    record CLOBBER. Flush any pending store for this slot first. Use this
    for effects that invalidate a cached value, such as a helper updating
    SP. Also invalidate the backend's physical cache flag; this function
    does not update native registers or backend-specific flags.

  pikecode_ssa_barrier(state, kind)
    Record kind in last_barrier and increment block. The kinds are
    PIKE_SSA_CALL, PIKE_SSA_BRANCH, PIKE_SSA_MERGE and PIKE_SSA_FUNCTION.
    All pending stores must already have been materialized. A barrier
    neither flushes stores nor clobbers registers: its kind labels the
    boundary, but does not apply call effects or merge predecessor state.

  pikecode_ssa_merge_reg(state, slot)
    Record PHI and make reg[] name the current memory[] version, without
    allocating a new value number or emitting a load. The backend must
    first prove that the incoming physical cache really holds that value
    on every edge. Materialize pending stores before the merge. This is
    an annotation of an established join, not a phi constructor that
    reconciles differing predecessor values.

PIKE_DEBUG checks detect stale loads, lost pending definitions, unmatched
stores and stores crossing barriers. Keep these invariants in non-debug
builds too; the checks do not perform the missing write-back or reload.

Example: load, update, publish, call
-----------------------------------

This schematic emitter shows two SP updates sharing one write-back. The
backend_emit_* and backend_invalidate_* helpers stand for native emitter
operations and physical-cache bookkeeping, not additional generic API.
The example assumes IP is already available, as reset requires.

  struct pikecode_ssa_state state;
  struct pikecode_ssa_op store;

  pikecode_ssa_reset(&state);
  backend_emit_load_sp();
  pikecode_ssa_read(&state, PIKE_SSA_SP);

  backend_emit_advance_sp();
  pikecode_ssa_define(&state, PIKE_SSA_SP);
  backend_emit_advance_sp();
  pikecode_ssa_define(&state, PIKE_SSA_SP);

  if (pikecode_ssa_take_store(&state, PIKE_SSA_SP, &store))
    backend_emit_store_sp();
  pikecode_ssa_barrier(&state, PIKE_SSA_CALL);
  backend_emit_call_that_updates_sp();
  pikecode_ssa_clobber(&state, PIKE_SSA_SP);
  backend_invalidate_sp_cache();

  backend_emit_load_sp();
  pikecode_ssa_read(&state, PIKE_SSA_SP);

A real sync helper must publish every dirty slot before the call, not only
SP. It must also publish the correct source PC for exceptions and account
for callee effects and ABI register clobbers. Existing opcode flags include
I_UPDATE_SP, I_UPDATE_M_SP and I_UPDATE_FP. Invalidate derived caches when
needed, including LOCALS if its base changes (e.g. stack relocation) or
its register is clobbered.
Invalidation can be recorded just before or just after the native call;
it must take effect before subsequent generated uses of the cached value.

See amd64_add_sp(), flush_dirty_regs(), amd64_ssa_barrier() and
sync_registers() for this protocol in an existing backend. SPARC uses
sparc_ssa_load/define/store() wrappers and SPARC_FLUSH_UNSTORED() to combine
the generic store protocol with its deferred stack-pointer bias.

Branches and joins
------------------

Materialize outgoing state before a branch. At a label, either invalidate
cached state and reload when needed, or retain only values proven valid
on every incoming edge. A fresh generic reset alone cannot supply that
proof: its version numbers restart and must not be compared with numbers
from earlier reset intervals or unrelated state objects.

In particular, knowing that a predecessor once loaded SP is insufficient.
Every retained predecessor must agree on the logical stack-pointer value
and relative depth, and deferred biases must be normalized before the
merge. Preserve the physical register only after establishing agreement,
then annotate the destination with pikecode_ssa_merge_reg(). Otherwise use
the conservative flush/invalidate path.

The shared assembler in ../peep.c supports the following backend hooks
for forward joins. Supply these hooks as a set:

  CODE_GENERATOR_STATE_VALID()
    Return a backend-defined mask of physically valid cached state.

  CODE_GENERATOR_SP_VALUE() and CODE_GENERATOR_SP_DEPTH()
    Return a stack identity and relative depth that remain meaningful
    across generic model resets. Existing backends track these separately
    from state.reg[PIKE_SSA_SP].

  CODE_GENERATOR_STATE_SP
    Identify the SP bit in the validity mask.

  FLUSH_CODE_GENERATOR_STATE_KEEP(keep)
    Publish the fallthrough state, invalidate other caches and retain
    only the proven incoming mask. The backend applies any additional
    restrictions before annotating retained registers in the new state.

The assembler intersects incoming masks, checks SP agreement and requires
all incoming edges to have been seen before retaining state. Unknown or
backward incoming edges use the conservative path. These hooks do not
provide loop-carried values or general phi resolution. Backend-local
labels and native branches need equivalent handling in their own emitters.

Adding SSA tracking to a backend
-------------------------------

Guard the integration with PIKE_ENABLE_SSA and retain the ordinary emitter
for other builds. Set up one model with the backend's function/label reset
logic, including its IP convention. Wrap native loads and updates with
read/define calls, maintain physical validity and flush pending stores at
all relevant sync points before applying a barrier. Apply clobbers for
calls and invalidate dependent caches. Begin with conservative label and
call handling before adding register preservation.

START_NEW_FUNCTION, END_FUNCTION and FLUSH_CODE_GENERATOR_STATE are the
general code-generator lifecycle hooks documented in README.txt. They are
not generic SSA operations: the backend must connect them to its model
and its native emitter. Likewise, implementing the forward-join hooks is
optional; the generic API does not enable them by itself.

The integer planners are another optional integration. A backend supplies
CODE_GENERATOR_INTEGER_ENABLED() and CODE_GENERATOR_INTEGER_RUN(code, n,
store_lines). The latter returns the number of consumed source opcodes,
or zero to use ordinary emission. It must guard inputs, lower the plan,
publish snapshots on exits, replay failures and restore source/PC state.
The assembler avoids integer regions while tracing or disassembling.
Native-region buffering uses CODE_GENERATOR_REGIONS_ENABLED() and the
CODE_GENERATOR_REGION_* hooks. Reusing the generic state model does not
require implementing that layer.

When extending an emitter, exercise branches, loops, callbacks, exceptions,
stack/mark-stack updates, source positions and non-SSA builds. Compare
optimized and conservative modes on each affected ABI. Check bignum and
UNDEFINED behavior for integer lowering. PIKE_SSA_STATS counts emission
and logical operations; execution profiling and timing are separate tools.

Integer value regions
---------------------

The integer value pass is enabled by default in SSA-enabled builds on
backends that implement it for the configured Pike integer width. Set
PIKE_SSA_NO_INTEGER_REGIONS=1 to disable it for comparisons or
debugging. The former PIKE_SSA_INTEGER_REGIONS flag is no longer needed.
The pass recognizes up to eight integer operations: additions,
increments, subtractions, and bitwise AND/OR/XOR using at most three
locals across adjacent source lines. The original planner supports local
updates and local/stack/add/assignment units that leave the Pike stack
at its original depth. Labels, debug directives, and unsupported operations end
a run. Tracing and disassembly use the ordinary emitter.

The shared planners in ssa_integer.[ch] assign SSA versions to integer
loads and arithmetic results and allocate logical value banks. The
original balanced-unit planner coalesces each local's versions into one
bank. Neither planner contains native registers or instruction
encodings. The backends lower the same plan independently. Each input
local is guarded once. A result replaces the current version only after
its overflow check, and only final dirty locals are stored on the fast
path. The balanced-unit path commits completed units on overflow and
resumes the remaining source opcodes through their ordinary slow paths.
A multi-step unit saves its input so it can restore that version and
replay the unit from a balanced Pike stack. Input allocation for an
unaccepted following unit is discarded, preserving the accepted prefix.
No fast-path operation calls C or exposes an uncommitted value to a
callback. Source locals that were
only read retain their original subtype tags.

Computed versions have ordinary integer tags. When a binary addition has
two untouched input versions, an entry guard requires one to have an
ordinary tag; otherwise replay preserves the all-UNDEFINED result. The
ordinary-tag check also checks the integer type in the same guard.

The balanced single-update path is enabled by default. Set
PIKE_SSA_NO_INTEGER_SINGLE_UPDATE=1 to disable it for comparisons. It
accepts one F_2_LOCALS / F_ADD_INTS / F_ASSIGN_LOCAL_AND_POP unit when the
assignment targets either input. Only the guarded integer plan commutes
operands; replay keeps their original order for other types and callbacks.
The existing type/subtype guards and overflow replay apply; successful
updates avoid all operand/result Pike-stack copies. Lone increments,
immediate additions, arbitrary destinations and live-stack exits retain
the existing two-operation/reuse thresholds. The former positive
PIKE_SSA_INTEGER_SINGLE_UPDATE flag is ignored.

Set PIKE_SSA_INTEGER_SINGLE_ASSIGN=1 to experiment with an isolated
F_2_LOCALS / F_ADD_INTS / F_ASSIGN_LOCAL_AND_POP assigning a third local,
such as c=a+b. It also accepts LOCAL / DUP(0) / ADD_INTS /
ASSIGN_LOCAL_AND_POP for c=a+a, using one guarded input version for both
operands. The arithmetic snapshot/source offset follows ADD_INTS after the
duplicate; replay retains all four original instructions. Other DUP depths
and assignments back to the input retain their existing handling.
Longer profitable stack plans retain priority. This exact exception requires
the stack-aware planner but no live-stack exit. It does not relax other
single-operation profitability limits.

Only the source locals are loaded as numeric inputs. The write_only mask
marks the separate destination slot: its entry guard checks only that the
old value owns no reference. Float and UNDEFINED destinations may take the
fast path; reference values replay ordinary assignment so their release
and possible destructor callbacks keep their original ordering. The dummy
initial destination version is absent from all snapshots. The result uses
its free bank, and only that destination is dirty at the successful exit.
An overflow may replay the whole source unit without materializing stack
operands, since neither locals nor the real stack have changed. Ordinary
source order, subtype guards and arithmetic error locations are preserved.

The stack-aware planner is enabled by default. It extends the original
path through retained local assignments (F_ASSIGN_LOCAL), with bounded
virtual Pike-stack operands and three shared value banks. Banks are
reused only after their last local or stack reference disappears. Set
PIKE_SSA_NO_INTEGER_STACK=1 to use only the original balanced-unit
planner for comparisons. It also remains the fallback when the extended
plan cannot fit its register budget.

A snapshot before each arithmetic opcode names the current local
versions, dirty locals, and virtual stack values. Speculative results
replace their bank only after the overflow check. An overflow exit
publishes the stack and dirty locals, then resumes at the failed opcode;
completed increments are not replayed. Input stack tags are copied
before local commits so an original UNDEFINED subtype survives even if
its local has been updated. The slow-path merge clears cached machine
state before each possible entry.

Successful exits may leave up to eight bounded virtual values above the
entry stack pointer when integer expressions are enabled (otherwise two).
The final snapshot publishes those values, their tags, the stack pointer,
and dirty locals before the next ordinary opcode. Both fast and replayed
paths leave machine caches empty at their merge. Set
PIKE_SSA_NO_INTEGER_STACK_EXITS=1 to require the original entry stack
depth at successful exits while retaining stack-aware overflow exits.

Regions still stop at file changes, debug directives and unsupported
opcodes, and do not carry registers across calls or labels. Native arithmetic
and slow-path opcodes retain their source records. Slow calls publish their
own PC, and the merge ensures a following call cannot reuse a PC from a
skipped fallback path. Bare input assignments that need parallel
subtype-tag moves use the ordinary path. Native encoding and register
mappings remain backend-specific.

The stack-aware planner lowers variable and immediate subtraction with
signed overflow checks. SUB nodes retain their original operands instead
of negating the right operand. This also represents an INT32_MIN
subtraction immediate without signed negation in the planner. Set
PIKE_SSA_NO_INTEGER_SUBTRACT=1 to restore the previous subtraction
handling; the original balanced-unit planner is unchanged.

The stack-aware planner also lowers variable and immediate AND, OR, and
XOR. Guarded native integer operands produce native-width logical
results without overflow, so these steps need no overflow branch or side
exit. Arithmetic before or after them retains its exact snapshots.
Logical results have ordinary integer tags, including zero; read-only
inputs keep their original subtype tags. Logical assignments can end a
balanced region without retaining a stack value. Set
PIKE_SSA_NO_INTEGER_BITWISE=1 to restore previous region handling.

The stack-aware planner lowers unary negation and bounded n-ary builtin
addition. It checks the actual callee identity; arbitrary calls remain
boundaries. Virtual integer literals need no physical register. Arguments
and local versions still share three banks, with at most eight virtual
stack operands and eight arithmetic operations. Every add in an n-ary sum
counts against that operation budget. A sum is one SSA value operation in
statistics, regardless of its number of native additions.

Negation checks native signed overflow. A sum checks each partial addition
before publishing its result; any overflow restores all original arguments
and replays the entire builtin call, including bignum promotion and later
cancellation. If every argument could be an untouched UNDEFINED input, an
entry tag guard requires one ordinary integer; otherwise replay retains
the builtin's all-UNDEFINED result. Read-only source subtypes are unchanged.

Set PIKE_SSA_NO_INTEGER_EXPRESSIONS=1 to disable these expression forms.
Fixed-arity builtin call selection is a separate backend capability and is
unchanged by integer-region comparison controls.

Constant-local assignments
--------------------------

The stack-aware planner keeps F_ASSIGN_LOCAL_NUMBER_AND_POP and
F_CLEAR_LOCAL inside existing regions. It also accepts retained assignments
of supported literal values. These writes create ordinary integer constant
versions without a physical bank or an arithmetic-operation budget charge;
they still count against the instruction/value bounds. The balanced planner
and region entry-opcode set are unchanged. Bare input copies still require
parallel subtype moves and remain unsupported.

All destination locals are guarded as integers at entry, including locals
first seen as write destinations. Fast-path assignments therefore release
no references and invoke no callbacks. Noninteger inputs replay ordinary
opcodes, preserving free_svalue() and destructor behavior. An overflow after
a constant assignment publishes that constant with an ordinary tag before
replaying the failing operation. Original input tags are copied to the stack
before dirty locals are committed, including locals replaced by constants.
Both final and overflow snapshots materialize constants explicitly.

Set PIKE_SSA_NO_INTEGER_CONSTANT_ASSIGN=1 to restore previous planning.
The feature works independently of expression/bitwise/live-stack-exit
controls; the existing minimum arithmetic count, three-bank limit and
label/file/call boundaries still apply.

Incoming evaluation-stack integers
----------------------------------

When the existing planner rejects a virtual-stack underflow, it can retry
with one integer already on the evaluation stack. Existing profitable plans
keep their handling. This exposes expressions such as a global integer plus
a negated local without lowering the global load or store itself.

The input occupies one of the three physical banks and requires an ordinary
integer type and subtype at entry. Undefined, noninteger and bignum inputs
replay the original instructions. The initial operand is not changed by
speculative arithmetic. Overflow snapshots restore the operands before the
failing operation; successful exits publish the final virtual stack.
Snapshots use entry SP minus one as their base, so a consumed input is
removed even when the final virtual stack is empty. Deeper slots remain
untouched. Guards, locals, native-operation budgets and control-flow
boundaries retain their existing restrictions.

Set PIKE_SSA_NO_INTEGER_STACK_INPUT=1 to restore previous planning. The
extension requires the stack-aware planner. Live-stack exits still require
the corresponding control. The direct-global extension below can include
a leading global load. Other global accesses retain their boundaries.

Direct-global integer operands
------------------------------

The direct-global extension defaults on in SSA-enabled builds on AMD64,
SPARC32 and SPARC64. Set PIKE_SSA_NO_INTEGER_GLOBAL_INPUT=1 to disable it.
The former positive flag PIKE_SSA_INTEGER_GLOBAL_INPUT is obsolete.
Indirect/inherited calls retain their incoming-stack fast path.

The stack planner can start at F_PRIVATE_IF_DIRECT_GLOBAL followed by an
eligible local-based expression. It keeps that global operand virtual in
one of the three value banks, avoiding its ordinary stack push and reload.
The minimum arithmetic count and operation/instruction limits are unchanged.

Entry guards require the current object's program to match the active
frame context's live program, and the global svalue to have an ordinary
integer type and subtype. A program mismatch enters indirect indexing using
that check's object and context; it preserves virtual dispatch and the
ordinary destructed-object error. A direct tag or local guard failure copies
the complete svalue from the already resolved storage, preserving references
and subtypes. Neither path repeats the program comparison. Both join the
preceding incoming-stack arithmetic plan with fresh guards after any getter
callback. Remaining slow operations keep ordinary replay. Direct storage
uses the object storage and context storage offset, including offsets beyond
the immediate load range.

Successful exits publish the normal virtual stack. Overflow snapshots
materialize the original global operand before replaying the failing
arithmetic, without repeating the producer. These snapshots are relative
to the untouched entry SP: this form consumes no incoming real stack slot.
The existing guarded incoming-stack planner remains independently usable.
Guard failures that enter its fallback produce separate profile entries;
a failed direct-global guard can therefore be followed by fast arithmetic.

Only the leading untyped direct-global load and the terminal store below
are included. Subsequent global reads, typed raw storage, calls and labels
retain their boundaries. The read itself changes no reference counts and
makes no callback on the fast path. PIKE_SSA_NO_INTEGER_GLOBAL_INPUT=1
forces the extension off for comparisons, including when the former positive
flag is set.

Terminal stores to the same global
----------------------------------

A direct-global region can include a terminal
F_ASSIGN_PRIVATE_IF_DIRECT_GLOBAL to the same raw offset and identifier
index as its input. Its final virtual stack must hold exactly one result.
An immediately following F_POP_VALUE can also be included. Both instructions
count against the producer-inclusive 32-instruction limit; the existing
arithmetic and bank limits remain. The store ends the region.

The entry guards prove that the old global is an ordinary integer in live
direct storage. No call or global write can intervene on the successful
path, so the backend commits just the integer payload after all arithmetic
checks pass. Computed and constant results are ordinary integers; storing
an unchanged local also requires its complete ordinary-integer entry tag.
Retained results are published normally, while popped results need no real
stack slot. Local assignments retain their existing snapshots.

Guard failures retain the incoming-stack arithmetic and original assignment.
Overflow snapshots leave the global unmodified until ordinary replay reaches
the assignment. Indirect setters, reference release, callbacks and destruction
keep their ordinary handling. Other destinations and typed stores remain
boundaries. Terminal stores default on with direct-global input. Set
PIKE_SSA_NO_INTEGER_GLOBAL_STORE=1 to restore the preceding store boundary
independently of the input control.

Runtime profiling
-----------------

Set PIKE_SSA_INTEGER_PROFILE=1 to emit execution counters for integer
regions. Normal builds emit no counter instructions when it is unset.
Each process reports entries, type-guard failures, overflow exits and
successful fast exits on stderr at exit. Separate counters record entries to
local-based candidates rejected by both planners. Counters are unsigned 64-bit
values, updated under the interpreter lock, and reset in a forked child.
Source names and records remain allocated until process exit, even after
their compiled programs have been reclaimed.

Executed regions also produce tab-separated rows with the PID, region ID,
program ID, escaped source file, first/last source lines, planner, operation
and local counts, following opcode, entries, guard failures, overflows and
fast exits. Non-ASCII/control characters in source names use ASCII escapes.
Rows also record the planner's stop reason, scan offset, stopping opcode
and line, rejected-prefix reason, and emitted prefix length. Stop reasons
separate unsupported operations/calls, control-flow and label boundaries,
source-file changes, disabled features, and operation/instruction/local/
stack/register budgets. Rejected-prefix reasons include too few arithmetic
operations, no load reuse, untouched inputs, and unsupported exit shapes.
stack-underflow means the virtual plan lacks an operand, which may already
exist on the real Pike stack; it does not indicate a runtime stack error.
The scan can look beyond the profitable emitted prefix: next_opcode is the
first unconsumed opcode, while stop_opcode identifies the scan failure.
The offsets in stop_offset and consumed make this distinction explicit.

Rejected candidates have their own rows and counters, even if a program
emits no integer regions. Candidates start at the same local-load/update
opcodes or eligible direct-global prefixes as planning; fused indexing,
other global operations and other starts are
outside this count. Rejected rows use ops for the discovered arithmetic
budget, not successful value operations. Candidate attempts may overlap.
Their counters run at entry, without type guards: they count opportunities,
not proven integer operations or executions of the stopping opcode. Earlier
exceptions or replay can prevent reaching it. Profiling flushes cached state
before rejected-candidate counters; use uninstrumented runs for timings.

The balanced planner reports unbalanced-unit when its balanced local-update
pattern does not match; the stack planner reports its finer scan constraints.
The fast_ops total counts operations in successful regions; it excludes
completed prefixes of overflowing regions and all ordinary opcodes.

Summarize one or more captured stderr logs with:
  pike tools/ssa_integer_profile.pike [--json] [--boundaries] [--top N] log...
Use - for standard input. The tool checks entry/exit counts and process
totals, and combines recompilations and processes at the same source site.
JSON includes all region sites, rejected sites and boundary aggregates;
--top limits the text output. --boundaries ranks stop reasons/opcodes by
region entries plus rejected candidate entries, keeping both counts separate
and reporting rejection reasons. Older logs remain readable as legacy rows.
For workload comparisons, filter by source and subtract a one-invocation
run from a two-invocation run to exclude startup and preparation counts.
Run the parser and runtime regression checks with:
  pike tools/ssa_integer_profile_test.pike [pike [Pike options...]]

These counters show which regions execute and fall back; they do not give
a denominator for all executed Pike operations. Use uninstrumented runs
for timing and sampling. Debug.get_program_layout(p, 1) additionally returns
the code
buffer base at index -1, allowing exact function bounds in profiler maps.

Emission statistics and benchmarking
-----------------------------------

Integer statistics count regions, value operations, SSA values,
reused logical operand reads, deferred local writes, stack-aware
regions, successful exits with live stack values, native subtractions,
and bitwise operations. Reused operand reads and deferred writes are IR
counts, not hardware load/store counts. Use perf and repeated runtime
tests for performance claims. Tools.Shoot.AdjacentArithmetics exercises
adjacent updates while masking results back into the native integer
range. For a paired comparison, run -x benchmark --json -s1 with
-t'Adjacent arithmetics (locals)' with PIKE_SSA_NO_INTEGER_REGIONS unset
and set.
