# Generic step-by-step explanation corpus

The native generator produces 550,000 original, deterministic explanation-training text rows by default:

**55 topics × 10,000 lines per topic = 550,000 lines.**

~~~
./build/pe-generate-explanations explanation-data 10000
~~~

Each JSONL row records a domain, topic, difficulty, explanation phase, and generic instructional text. The five phases are interpret, prepare, transform, present, and verify. Text explicitly encourages named rules, intermediate values, assumptions, and an independent correctness check.

This corpus is intended as an augmentation resource for future explanation models and UI copy. It is not a set of fabricated solved answers, and it must not replace the native solver or verifier as the source of truth.

The generated explanation-data directory is intentionally excluded from Git because it is reproducible and large. The generator is C++ source in tools/generate_explanation_corpus.cpp; regenerate it locally or in a release pipeline.
