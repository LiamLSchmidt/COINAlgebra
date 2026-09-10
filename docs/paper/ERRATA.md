# Paper–code discrepancy register

This register tracks manuscript typos, implementation differences, and unresolved
questions. The paper is the mathematical reference; a code difference is not
by itself evidence that the paper is wrong. Preserve entries when resolved,
recording the resolution and the manuscript version in which it appears.

Source version: `APS_Coincidence_Algebra-14.pdf`.
Started: 2026-09-08. The manuscript PDF has not been edited.

## Review process

After the first build is agreed ready for validation, audit definitions,
equations, conventions, and examples against the code and tests. Record the
code commit and manuscript version used for that audit. Until then this is an
incremental register, not a claim of complete paper–code agreement.

For each finding, keep a stable ID, paper location/version, printed statement,
code behavior, evidence, status, and resolution. Distinguish confirmed typos,
suspected typos, deliberate extensions, and unresolved discrepancies. Link
regression tests where available. Do not silently resolve differences by
changing either the paper or the implementation.

## PA-001 — Detector-count exponent

- **Location:** version 14, Eq. (31), used by Eq. (32).
- **Status:** Confirmed typo; corrected convention agreed with the author on
  2026-09-08. Manuscript update pending.
- **Printed:** denominator N^(k+1) in the power expansion.
- **Agreed:** for k observed gammas, use N^(k-1). The positive-power factor in
  Eq. (32) is H = h + h^2/N + h^3/N^2 + ... . If the expansion includes an
  identity, give it coefficient one and remove it before forming H.
- **Reason:** the first gamma can hit any detector; each additional gamma must
  reach that same detector. This assumes whole-array efficiencies, identical
  detectors, and isotropic emission.
- **Code:** `DetectionMaps::SummingInExpansion` in
  `src/Detection/DetectionMaps.cxx`.
- **Evidence:** `tests/core/test_summing.C` checks N=1, N=2, N=4, and independent
  cascade enumeration. The single-gamma term is not divided by N.

## PA-002 — Lower propagation boundary

- **Location:** version 14, Eq. (32).
- **Status:** Unresolved paper–code discrepancy; proposed boundary clarification.
  Not yet classified as an author-confirmed typo.
- **Printed:** lower factor V_source(O), with O = 1 + o + o^2 + ... .
- **Code:** V_source(O E), where E is the sum of stationary paths at quiver
  sinks. Only completed downstream cascades contribute.
- **Reason for current code:** for an upper observed edge followed by a single
  lower edge, unrestricted projection produces 1 + o_lower, whereas requiring
  the lower gamma to avoid the detector gives o_lower. The unrestricted sum
  counts both partial and completed suffixes.
- **Code:** `DecayProbability::SummingFeedingVector` in
  `src/Probability/DecayProbability.cxx`.
- **Evidence:** lower-edge summing-out and multiple-terminal cascade enumeration
  in `tests/core/test_summing.C`.
- **Review needed:** verify the manuscript projector/expansion definitions and
  intended termination convention. Agree the treatment of terminal/metastable
  levels and observation windows before resolving this entry.

## CA-001 — Target of the final coincidence factor

- **Location:** version 14, Eq. (40).
- **Status:** Previously identified suspected typo; deferred to the later
  coincidence-algebra audit. Not author-confirmed in the current discussion.
- **Recorded discrepancy:** `docs/paper/README.md` reports that the printed
  target uses the source of the final factor; the code uses its target.
- **Code:** `DecayCoin::GetTarget` in `src/Core/DecayCoin.cxx`.
- **Existing rationale:** consistency with the path embedding and Eq. (41).
- **Existing coverage:** `tests/core/test_coin.C`.
- **Review needed:** verify the printed equation and corresponding tests during
  the coincidence-algebra audit. This entry preserves the earlier finding;
  it does not expand the present path-algebra scope.

## PA-003 — Explicit internal-conversion response

- **Location:** version 14, detection maps in Section II.B, Eqs. (28)-(30).
- **Status:** Implementation convention to reconcile during the audit; not a
  claimed manuscript typo.
- **Code convention:** physical edges contain gamma-plus-conversion branching.
  Efficiencies are conditional on gamma emission. With q = 1/(1+alpha), the
  hit factor is q * peak_efficiency and the avoidance factor is
  1 - q * total_efficiency/N.
- **Review needed:** state whether the paper absorbs the gamma-emission
  fraction into its efficiencies or requires q explicitly, preventing an
  omitted or duplicated conversion correction.
- **Code and evidence:** `DetectionMaps` and `tests/core/test_summing.C`;
  see `docs/mathematics/internal-conversion.md` and
  `docs/mathematics/detection-summing.md`.

## CA-002 — Two-detector gated avoidance and group labels

- **Location:** version 14, Eqs. (71)-(72), pages 10-11.
- **Status:** Concrete counterexample for the interpretation “two distinct clean
  full-energy peaks”; observable/general formula needs author review.
- **Printed:** products use the one-detector summing-out fiber, with gated hit
  factor h*(N-1)/N. This correctly chooses a different detector for a second
  single photon, but the o fiber protects only one detector from other photons.
- **Counterexample:** a deterministic three-photon cascade, perfect peak/total
  efficiencies, no IC. Select the first two individual photons as clean peaks.
  The third must avoid both detectors. Enumerating assignments gives
  (N-1)(N-2)/N^2, whereas the degree-two term of the printed expression gives
  (N-1)^2/N^2. For N=2 these are 0 and 1/4; for N=4, 3/8 and 9/16.
- **Implementation:** `AvoidDetectors(tau,2)` supplies 1-2*q*total/N on hidden
  edges; a restricted resolved-pair calculation is demonstrated in
  `examples/calculations/coincidence.C`. General summed-in gated groups are not
  implemented. Their detector partition must be retained rather than inferred
  from a tensor that has already combined distinct detection scenarios.
- **Evidence:** `tests/core/test_coincidence_detection.C` checks physical and
  response-weighted cascade subsets and explicitly enumerates detector
  assignments for this counterexample. No manuscript edits made.

## CA-003 — Missing initial population in Table III

- **Location:** version 14, page 10, single-hit row epsilon_2 tensor p_10,
  column f_2.
- **Status:** Suspected table typo, not author-confirmed.
- **Printed:** o_21*h_10, without f_2.
- **Eq. (70) and code:** f_2*o_21*h_10. All contributions are linear in the
  initial population; the row must vanish if f_2=0.
- **Evidence:** mixed-population enumeration in
  `tests/core/test_coincidence_detection.C`.
