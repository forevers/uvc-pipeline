
#pragma once

const char g_schema[] ="{\
  \"$schema\": \"http://json-schema.org/draft-04/schema#\",\
  \"$id\": \"http://ess.com/product.schema.json\",\
  \"title\": \"ess camera pipeline\",\
  \"description\": \"ess camera enumeration\",\
  \"type\": \"object\",\
  \"properties\": {\
    \"cameras\": {\
      \"type\": \"array\",\
      \"items\":\
      {\
        \"type\": \"object\",\
        \"properties\": {\
          \"device node\": {\
            \"type\": \"string\"\
          },\
          \"card type\": {\
            \"type\": \"string\"\
          },\
          \"formats\": {\
            \"type\": \"array\",\
            \"items\": [\
              {\
                \"type\": \"object\",\
                \"properties\": {\
                  \"index\": {\
                    \"type\": \"integer\"\
                  },\
                  \"type\": {\
                    \"type\": \"string\"\
                  },\
                  \"format\": {\
                    \"type\": \"string\"\
                  },\
                  \"name\": {\
                    \"type\": \"string\"\
                  },\
                  \"res rates\": {\
                    \"type\": \"array\",\
                    \"items\":\
                    {\
                      \"type\": \"object\",\
                      \"properties\": {\
                        \"resolution\": {\
                          \"type\": \"string\"\
                        },\
                        \"rates\": {\
                          \"type\": \"array\",\
                          \"items\":\
                          {\
                            \"type\": \"string\"\
                          },\
                          \"minItems\": 1\
                        }\
                      },\
                      \"required\": [\
                        \"resolution\",\
                        \"rates\"\
                      ]\
                    },\
                    \"minItems\": 1\
                  }\
                },\
                \"required\": [\
                  \"index\",\
                  \"type\",\
                  \"format\",\
                  \"name\",\
                  \"res rates\"\
                ]\
              }\
            ]\
          }\
        },\
        \"required\": [\
          \"device node\",\
          \"card type\",\
          \"formats\"\
        ]\
      },\
      \"minItems\": 1\
    }\
  },\
  \"required\": [\
    \"cameras\"\
  ]\
}";