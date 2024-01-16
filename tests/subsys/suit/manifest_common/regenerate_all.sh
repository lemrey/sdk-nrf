#!/bin/bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

declare -A envelopes=(
  ["../orchestrator/manifest/sample_unsupported_component_54.yaml"]="../orchestrator/src/manifest_unsupported_component_54.c"
  ["../orchestrator/manifest/sample_unsupported_component.yaml"]="../orchestrator/src/manifest_unsupported_component.c"
  ["../orchestrator/manifest/sample_zero_54.yaml"]="../orchestrator/src/manifest_zero_size_54.c"
  ["../orchestrator/manifest/sample_zero.yaml"]="../orchestrator/src/manifest_zero_size.c"
  ["../fetch_integrated_payload_flash/manifest/manifest_52.yaml"]="../fetch_integrated_payload_flash/src/manifest_52.c"
  ["../fetch_integrated_payload_flash/manifest/manifest_54.yaml"]="../fetch_integrated_payload_flash/src/manifest_54.c"
)

declare -A envelopes_v2=(
  ["../orchestrator/manifest/sample_wrong_manifest_version_54.yaml"]="../orchestrator/src/manifest_wrong_version_54.c"
  ["../orchestrator/manifest/sample_wrong_manifest_version.yaml"]="../orchestrator/src/manifest_wrong_version.c"
)

declare -A envelopes_wrong_key=(
  ["../orchestrator/manifest/sample_valid_54.yaml"]="../orchestrator/src/manifest_different_key_54.c"
  ["../orchestrator/manifest/sample_valid.yaml"]="../orchestrator/src/manifest_different_key.c"
)

declare -A dependency_envelopes=(
  ["../storage/manifest/manifest_app.yaml"]="../storage/src/manifest_app.c"
  ["../storage/manifest/manifest_app_v2.yaml"]="../storage/src/manifest_app_v2.c"
  ["../storage/manifest/manifest_app_posix.yaml"]="../storage/src/manifest_app_posix.c"
  ["../storage/manifest/manifest_app_posix_v2.yaml"]="../storage/src/manifest_app_posix_v2.c"
  ["../storage/manifest/manifest_rad.yaml"]="../storage/src/manifest_rad.c"
  ["../storage/manifest/manifest_rad_v2.yaml"]="../storage/src/manifest_rad_v2.c"
  ["../storage/manifest/manifest_sys.yaml"]="../storage/src/manifest_sys.c"
  ["../orchestrator/manifest/sample_valid_54.yaml"]="../orchestrator/src/manifest_valid_app_54.c"
  ["../orchestrator/manifest/sample_valid.yaml"]="../orchestrator/src/manifest_valid_app.c"
)
declare -A root_envelopes=(
  ["../storage/manifest/manifest_root.yaml"]="../storage/src/manifest_root.c"
  ["../storage/manifest/manifest_root_v2.yaml"]="../storage/src/manifest_root_v2.c"
  ["../storage/manifest/manifest_root_posix.yaml"]="../storage/src/manifest_root_posix.c"
  ["../storage/manifest/manifest_root_posix_v2.yaml"]="../storage/src/manifest_root_posix_v2.c"
  ["../orchestrator/manifest/sample_valid_root_54.yaml"]="../orchestrator/src/manifest_valid_54.c"
  ["../orchestrator/manifest/sample_valid_root.yaml"]="../orchestrator/src/manifest_valid.c"
)
declare -A envelope_dependency_names=(
  ["../storage/manifest/manifest_app.yaml"]="app.suit"
  ["../storage/manifest/manifest_app_v2.yaml"]="app_v2.suit"
  ["../storage/manifest/manifest_app_posix.yaml"]="app_posix.suit"
  ["../storage/manifest/manifest_app_posix_v2.yaml"]="app_posix_v2.suit"
  ["../storage/manifest/manifest_rad.yaml"]="rad.suit"
  ["../storage/manifest/manifest_rad_v2.yaml"]="rad_v2.suit"
  ["../storage/manifest/manifest_sys.yaml"]="sys.suit"
  ["../storage/manifest/manifest_root.yaml"]="root.suit"
  ["../storage/manifest/manifest_root_v2.yaml"]="root_v2.suit"
  ["../storage/manifest/manifest_root_posix.yaml"]="root_posix.suit"
  ["../storage/manifest/manifest_root_posix_v2.yaml"]="root_posix_v2.suit"
  ["../orchestrator/manifest/sample_valid_54.yaml"]="sample_app.suit"
  ["../orchestrator/manifest/sample_valid.yaml"]="sample_app_posix.suit"
  ["../orchestrator/manifest/sample_valid_root_54.yaml"]="sample_root.suit"
  ["../orchestrator/manifest/sample_valid_root.yaml"]="sample_root_posix.suit"
)


# Regenerate regular envelopes
for envelope_yaml in "${!envelopes[@]}"; do
  envelope_c=${envelopes[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done


# Regenerate hierarchical manifests for the SUIT storage test
for envelope_yaml in "${!dependency_envelopes[@]}"; do
  envelope_c=${dependency_envelopes[$envelope_yaml]}
  envelope_name=${envelope_dependency_names[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  if [ -e ${envelope_c} ]; then
    python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
  fi
  mv sample_signed.suit ${envelope_name}
done
for envelope_yaml in "${!root_envelopes[@]}"; do
  envelope_c=${root_envelopes[$envelope_yaml]}
  envelope_name=${envelope_dependency_names[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
  mv sample_signed.suit ${envelope_name}
done


# Manually manipulate some of the generated envelopes to match test description
cp "../orchestrator/src/manifest_valid_54.c" "../orchestrator/src/manifest_manipulated_54.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' "../orchestrator/src/manifest_manipulated_54.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' "../orchestrator/src/manifest_manipulated_54.c"
sed -i 's/.yaml/.yaml\n *\n * @details The manipulation is done on byte of index 7, from value 0x58 to 0xFF/g' "../orchestrator/src/manifest_manipulated_54.c"
sed -i 's/manifest_valid_buf/manifest_manipulated_buf/g' "../orchestrator/src/manifest_manipulated_54.c"
sed -i 's/manifest_valid_len/manifest_manipulated_len/g' "../orchestrator/src/manifest_manipulated_54.c"

cp "../orchestrator/src/manifest_valid.c" "../orchestrator/src/manifest_manipulated.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' "../orchestrator/src/manifest_manipulated.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' "../orchestrator/src/manifest_manipulated.c"
sed -i 's/.yaml/.yaml\n *\n * @details The manipulation is done on byte of index 7, from value 0x58 to 0xFF/g' "../orchestrator/src/manifest_manipulated.c"
sed -i 's/manifest_valid_buf/manifest_manipulated_buf/g' "../orchestrator/src/manifest_manipulated.c"
sed -i 's/manifest_valid_len/manifest_manipulated_len/g' "../orchestrator/src/manifest_manipulated.c"

# Generate envelopes with modified manifest version number
sed -i 's/suit-manifest-version         => 1,/suit-manifest-version         => 2,/g' "../../../../../modules/lib/suit-processor/cddl/manifest.cddl"
for envelope_yaml in "${!envelopes_v2[@]}"; do
  envelope_c=${envelopes_v2[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done
sed -i 's/suit-manifest-version         => 2,/suit-manifest-version         => 1,/g' "../../../../../modules/lib/suit-processor/cddl/manifest.cddl"

SUIT_GENERATOR_DIR="../../../../../modules/lib/suit-generator"
KEYS_DIR=${SUIT_GENERATOR_DIR}/ncs

# Generate envelopes and sign them with new, temporary key
suit-generator keys --output-file key
mv ${KEYS_DIR}/key_private.pem ${KEYS_DIR}/key_private.pem.bak
mv key_priv.pem ${KEYS_DIR}/key_private.pem

for envelope_yaml in "${!envelopes_wrong_key[@]}"; do
  envelope_c=${envelopes_wrong_key[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done

mv ${KEYS_DIR}/key_private.pem.bak ${KEYS_DIR}/key_private.pem
